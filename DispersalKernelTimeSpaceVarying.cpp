
#include "stdafx.h"
#include "cfgutils.h"
#include <algorithm>
#include "Landscape.h"
#include "DispersalKernelTimeSpaceVarying.h"



DispersalKernelTimeSpaceVarying::DispersalKernelTimeSpaceVarying() {

}


DispersalKernelTimeSpaceVarying::~DispersalKernelTimeSpaceVarying() {
}

int DispersalKernelTimeSpaceVarying::init(int iKernel) {

	char szKeyName[_N_MAX_STATIC_BUFF_LEN];

	

	sprintf(szKeyName, "Kernel_%d_KernelManifestFileName", iKernel);
	char szKernelManifestFileName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szKernelManifestFileName, "P_KernelManifest.txt");
	readValueToProperty(szKeyName, szKernelManifestFileName, -1, "#FileName#");

	sprintf(szKeyName, "Kernel_%d_KernelChangeTimesFileName", iKernel);
	char szKernelChangeTimesFileName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szKernelChangeTimesFileName, "P_KernelChangeTimes.txt");
	readValueToProperty(szKeyName, szKernelChangeTimesFileName, -1, "#FileName#");

	//Data loading mode options:
	loadMode = DataLoadingMode::GRADUAL;

	const char *DataLoadingModeMonolithic = "MONOLITHIC";
	const char *DataLoadingModeGradual = "GRADUAL";
	const char *DataLoadingModeRolling = "ROLLING";

	sprintf(szKeyName, "Kernel_%d_KernelDataLoadMode", iKernel);
	char szKernelDataLoadMode[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szKernelDataLoadMode, "%s", DataLoadingModeGradual);
	char possibleValues[1024];
	sprintf(possibleValues, "{%s, %s, %s}", DataLoadingModeMonolithic, DataLoadingModeGradual, DataLoadingModeRolling);
	readValueToProperty(szKeyName, szKernelDataLoadMode, -1, possibleValues);//TODO: Clever caching-most-used few, dynamically-load-others one

	int bFoundAny = 0;
	if (strcmp(szKernelDataLoadMode, DataLoadingModeMonolithic) == 0) {
		printk("Using %s kernel data loading\n", DataLoadingModeMonolithic);
		loadMode = DataLoadingMode::MONOLITHIC;
		bFoundAny++;
	}

	if (strcmp(szKernelDataLoadMode, DataLoadingModeGradual) == 0) {
		//This is the default
		printk("Using %s kernel data loading\n", DataLoadingModeGradual);
		loadMode = DataLoadingMode::GRADUAL;
		bFoundAny++;
	}

	if (strcmp(szKernelDataLoadMode, DataLoadingModeRolling) == 0) {
		printk("Using %s kernel data loading\n", DataLoadingModeRolling);
		loadMode = DataLoadingMode::ROLLING;
		bFoundAny++;
	}

	if (!bFoundAny) {
		reportReadError("ERROR: %s was not a valid specification. Read \"%s\". Valid options are %s\n", szKeyName, szKernelDataLoadMode, possibleValues);
	}

	nMaxLoadedKernels = 1000000000;
	sprintf(szKeyName, "Kernel_%d_MaxLoadedKernels", iKernel);
	readValueToProperty(szKeyName, &nMaxLoadedKernels, -1, "~i>0~");
	if (nMaxLoadedKernels <= 0) {
		reportReadError("ERROR: MaxLoadedKernels must be positive. Read %d\n", nMaxLoadedKernels);
	}


	if (!bIsKeyMode()) {

		//Get the manifest of kernels
		int nKernels;
		if (!readIntFromCfg(szKernelManifestFileName, "nKernels", &nKernels)) {
			reportReadError("ERROR: Unable to read number of kernels from manifest file %s\n", szKernelManifestFileName);
		}

		std::vector<int> kernelIDs(nKernels);
		std::vector<std::string> kernelFileNames(nKernels);

		if (!readTable(szKernelManifestFileName, nKernels, 1, &kernelIDs[0], &kernelFileNames[0])) {
			reportReadError("ERROR: Unable to read kernel ID no.s and filenames from manifest file %s\n", szKernelManifestFileName);
		}

		//Get the time/zone changeovers:
		int nTimes;
		if (!readIntFromCfg(szKernelChangeTimesFileName, "nTimes", &nTimes)) {
			reportReadError("ERROR: Unable to read number of time points from file %s\n", szKernelChangeTimesFileName);
		}

		if (nTimes <= 0) {
			reportReadError("ERROR: Number of time points (%d) from file %s was <= 0 \n", nTimes, szKernelChangeTimesFileName);
		}

		std::vector<std::string> timeZoneFileNames(nTimes);
		changeTimes.resize(nTimes);

		if (!readTable(szKernelChangeTimesFileName, nTimes, 1, &changeTimes[0], &timeZoneFileNames[0])) {
			reportReadError("ERROR: Unable to read kernel ID no.s and filenames from manifest file %s\n", szKernelChangeTimesFileName);
		}

		//Read the zone files:
		timeZones.resize(nTimes);
		for (size_t iTime = 0; iTime < nTimes; ++iTime) {
			if (!timeZones[iTime].init(timeZoneFileNames[iTime])) {
				reportReadError("ERROR: Unable to read kernel zoning file %s\n", timeZoneFileNames[iTime].c_str());
			}
		}


		//Kernels don't need to be on the same extent as the landscape, and there is no way to check if they're correct
		RasterHeader<int> landscapeHeader = *(getWorld()->header);

		//hash of all kernels that were declared:
		std::unordered_map<int, int> declaredKernelIDs;
		for (size_t ikernelID = 0; ikernelID < kernelIDs.size(); ++ikernelID) {
			int kernelID = kernelIDs[ikernelID];
			if (declaredKernelIDs.find(kernelID) != declaredKernelIDs.end()) {
				reportReadError("ERROR: Kernel manifest file %s lists kernelID %d more than once", szKernelManifestFileName, kernelID);
			}
			declaredKernelIDs[kernelID] = 1;
			kernelIDtoFileName[kernelID] = kernelFileNames[ikernelID];
		}

		//Make a hash of all kernels that are actually ever used:
		std::unordered_map<int, int> referencedKernelIDs;

		//Check that the zone files don't reference any kernels that don't exist
		for (size_t iZoning = 0; iZoning < timeZones.size(); ++iZoning) {
			//Look at every single value in the zoning file and check that it is a associated with a kernel:
			auto &zoneRaster = timeZones[iZoning];

			//Check that zone files are on the right extent
			if (!zoneRaster.header.sameAs(landscapeHeader)) {
				reportReadError("ERROR: Kernel zoning raster %s is not on the same extent as the landscape\n", timeZoneFileNames[iZoning].c_str());
			}

			for (int iY = 0; iY < zoneRaster.header.nRows; ++iY) {
				for (int iX = 0; iX < zoneRaster.header.nCols; ++iX) {

					int kernelID = zoneRaster(iX, iY);

					referencedKernelIDs[kernelID] = 1;

					if (declaredKernelIDs.find(kernelID) == declaredKernelIDs.end()) {
						reportReadError("ERROR: Kernel Zone file %s references a kernelID %d not specified in the manifest file %s", timeZoneFileNames[iZoning].c_str(), kernelID, szKernelManifestFileName);
					}

				}
			}
		}

		if (loadMode == DataLoadingMode::MONOLITHIC) {

			if (referencedKernelIDs.size() > nMaxLoadedKernels) {
				printk("WARNING: Using monolithic loading mode and number of kernels exceeds MaxLoadedKernels.\nThis does not make sense, so switching to GRADUAL loading mode.\n");
				loadMode = DataLoadingMode::GRADUAL;
			} else {
				//Read in kernels at the start, but only those which are actually referenced:
				int iFile = 0;
				for (auto &kv : referencedKernelIDs) {
					int kernelID = kv.first;
					printf("Reading Dispersal Kernel from file %s (%d/%lu)\n", kernelIDtoFileName[kernelID].c_str(), iFile + 1, (unsigned long)referencedKernelIDs.size());
					kernels[kernelID] = KernelRasterPoints(kernelIDtoFileName[kernelID].c_str());
					++iFile;
				}
			}

		}


		//Check that the times are valid
		//Cover all the simulation times:
		//start at or before start time:
		if (changeTimes[0] > getWorld()->timeStart) {
			reportReadError("ERROR: Kernel changing times specified in file %s do not start before beginning of simulation (%f)", szKernelChangeTimesFileName, changeTimes[0]);
		}

		//strictly ascending:
		for (size_t iTime = 1; iTime < changeTimes.size(); ++iTime) {
			if (changeTimes[iTime] <= changeTimes[iTime - 1]) {
				reportReadError("ERROR: Kernel changing times specified in file %s are not strictly ascending: line %d %f --> %f", szKernelChangeTimesFileName, changeTimes[iTime - 1], changeTimes[iTime]);
			}
		}

	}

	reset();

	return 1;
}

void DispersalKernelTimeSpaceVarying::reset() {

	LoadModeRollingCurrentTimeIndex = changeTimes.size();

	return;
}

int DispersalKernelTimeSpaceVarying::changeParameter(char * szKeyName, double dNewParameter, int bTest) {
	return 0;
}

double DispersalKernelTimeSpaceVarying::KernelShort(int nDx, int nDy) {
	return 0.0;
}

int DispersalKernelTimeSpaceVarying::getShortRange() {
	return 0;
}

void DispersalKernelTimeSpaceVarying::KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double & dDx, double & dDy) {

	//uper_bound finds the first element which is greater than the value
	auto changeTimeIt = std::upper_bound(changeTimes.begin(), changeTimes.end(), tNow);

	//we want the last element less than or equal to the value (note this is not lower_bound)
	//changeTimes must start at or before timeStart, and tNows are always >= timeStart, so we can always decrement, as upper_bound can never return the first element
	//also, if tNow is greater than the last time, we would have been given changeTimes.end(), and so as desired this brings us back to the last element
	--changeTimeIt;

	size_t changeIndex = changeTimeIt - changeTimes.begin();

	auto &zoneIDRaster = timeZones[changeIndex];

	auto sourceID = zoneIDRaster(int(sourceX), int(sourceY));

	if (loadMode == DataLoadingMode::ROLLING) {
		if (changeIndex != LoadModeRollingCurrentTimeIndex) {
			//Throw out all loaded kernels not needed for the current time section:
			//TODO: In most common cases, this will be all of them, so might be generally quicker to not do this "clever" thing, as then can drop loops over zoneIDRaster

			//Find out what elements are needed for this time section
			std::unordered_map<int, int> usedIDs;

			for (int iY = 0; iY < zoneIDRaster.header.nRows; ++iY) {
				for (int iX = 0; iX < zoneIDRaster.header.nCols; ++iX) {
					int kernelID = zoneIDRaster(iX, iY);
					usedIDs[kernelID] = 1;
				}
			}

			//erase all but these
			for (auto it = kernels.cbegin(); it != kernels.cend() /* not hoisted */; /* no increment */) {
				auto kernelID = (*it).first;
				if (usedIDs.find(kernelID) == usedIDs.end()) {
					kernels.erase(it++);
				} else {
					++it;
				}
			}

			//Specify that all currently loaded kernels are valid for this time window:
			LoadModeRollingCurrentTimeIndex = changeIndex;
		}
	}

	if (loadMode != DataLoadingMode::MONOLITHIC) {

		//If we have too many kernels, then purge them in some (clever) way
		//TODO: Clever way, for now, just ditch them all.
		if (kernels.size() > nMaxLoadedKernels) {
			kernels.clear();
		}

		//See if we need to load this kernel:
		if (kernels.find(sourceID) == kernels.end()) {
			kernels[sourceID] = KernelRasterPoints(kernelIDtoFileName[sourceID].c_str());
		}
	}

	auto &kernelRaster = kernels[sourceID];

	//Pass necessary information on to the specific kernel:
	kernelRaster.kernelVirtualSporulation(tNow, sourceX, sourceY, dDx, dDy);

}

double DispersalKernelTimeSpaceVarying::getVirtualSporulationProportion() {
	return 1.0;
}

std::vector<double>CALL DispersalKernelTimeSpaceVarying::getCumulativeSumByRange() {
	std::vector<double> cumSum(1);
	cumSum[0] = 1.0;

	return cumSum;
}

void DispersalKernelTimeSpaceVarying::writeSummaryData(char * szSummaryFile) {



	return;
}
