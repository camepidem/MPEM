//InterOutputDataDPC.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "PopulationCharacteristics.h"
#include "Population.h"
#include "SummaryDataManager.h"
#include "DispersalManager.h"
#include "ListManager.h"
#include "InterOutputDataDPC.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterOutputDataDPC::InterOutputDataDPC() {

	timeSpent = 0;
	InterName = "OutputDataDPC";

	//Memory housekeeping:
	szLineBuffer = NULL;

	setCurrentWorkingSubection(InterName, CM_SIM);

	enabled = 1;
	//Read Data:
	frequency = (world->timeEnd - world->timeStart) / 100.0;
	if (readValueToProperty("DPCFrequency", &frequency, 1, ">0")) {
		if (frequency <= 0.0) {
			printk("DPCFrequency negative, no Disease progress curve data will be recorded\n");
			//Make it so intervention does not occur
			frequency = 1e30;
		}
	}

	readValueToProperty("DPCTimeFirst", &timeFirst, 1, "");
	if (timeFirst < world->timeStart) {
		double periodsBehind = (world->timeStart - timeFirst) / frequency;
		if (periodsBehind - floor(periodsBehind) < 0.01 * frequency) {//Avoid floating point rounding shifting by a period when we should be exactly aligned to timeStart
			periodsBehind = floor(periodsBehind);
		}
		int nPeriodsBehind = ceil(periodsBehind);
		double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

		timeFirst = newTimeFirst;
	}


	bQuiet = 0;
	readValueToProperty("DPCQuiet", &bQuiet, -1, "[0,1]");

	bSampleKernelStatistics = 0;
	readValueToProperty("DPCSampleKernelStatistics", &bSampleKernelStatistics, -1, "[0,1]");

	bBufferDPCOutput = 0;
	readValueToProperty("DPCBufferOutput", &bBufferDPCOutput, -1, "[0,1]");

	//Set up intervention:
	timeNext = timeFirst;

	if (!world->bGiveKeys) {

		nWeightings = SummaryDataManager::getNWeightings();
		nZonings = 0;
		nZones = 0;
		for (auto iPublicZoning : SummaryDataManager::getPublicZonings()) {
			nZonings++;
			nZones += SummaryDataManager::getNZones(iPublicZoning);
		}

		setupOutput();

	}
}

InterOutputDataDPC::~InterOutputDataDPC() {

	clearMemory();

}

int InterOutputDataDPC::makeQuiet() {
	bQuiet = 1;
	return 1;
}

int InterOutputDataDPC::clearMemory() {

	if (szLineBuffer != NULL) {
		delete[] szLineBuffer;
		szLineBuffer = NULL;
	}

	if (pDPCFiles.size() != 0) {
		for (int iWeight = 0; iWeight < nWeightings; iWeight++) {
			if (pDPCFiles[iWeight] != NULL) {
				fclose(pDPCFiles[iWeight]);
			}
		}
		pDPCFiles.resize(0);
	}

	if (pZoningDPCFiles.size() != 0) {
		for (int iZoning = 0; iZoning < nZonings; ++iZoning) {

			auto publicZonings = SummaryDataManager::getPublicZonings();

			int iSummaryZoning = publicZonings[iZoning];

			int nZonesThisZoning = SummaryDataManager::getNZones(iSummaryZoning);

			for (int iZoneThisZoning = 0; iZoneThisZoning < nZonesThisZoning; ++iZoneThisZoning) {
				fclose(pZoningDPCFiles[iZoning][iZoneThisZoning]);
			}
			pZoningDPCFiles[iZoning].resize(0);
		}
		pZoningDPCFiles.resize(0);
	}

	if (pKernelDataFiles.size() != 0) {
		for (int i = 0; i < DispersalManager::nKernels; i++) {
			fclose(pKernelDataFiles[i]);
		}
	}

	return 1;

}

int InterOutputDataDPC::setupOutput() {

	clearMemory();

	//Initialise buffer for data output:
	//Each line has Time, nHosts*(nClasses, nPopSus, nLPopInf), nLSus, nLInf
	//Allow 32 char per datum
	int maxOutputWidth = 32;

	int nCellMetrics = 3;//nSus, nInf, nUniqueInf

	int nTargetColumns = 1;//Time column

	for (int i = 0; i < PopulationCharacteristics::nPopulations; i++) {
		nTargetColumns += Population::pGlobalPopStats[i]->nTotalClasses + nCellMetrics;//+ List class data: nSus, nInf, nUniqueInf
	}

	if (PopulationCharacteristics::nPopulations > 1) {
		//Only give location metrics when have more than one population, otherwise redundant:
		nTargetColumns += nCellMetrics;//Location counts: nLSus, nLInf, nLUniqueInf
	}

	int nTargetBufferSize = nTargetColumns*maxOutputWidth;

	if (nTargetBufferSize > _N_MAX_DYNAMIC_BUFF_LEN) {
		reportReadError("ERROR: Unable to allocate DPC buffer. Too Many headers\n");
	}

	szLineBuffer = new char[_N_MAX_DYNAMIC_BUFF_LEN];


	//make DPC file and write its header
	pDPCFiles.resize(nWeightings);

	for (int iWeight = 0; iWeight < nWeightings; iWeight++) {

		char szWeightName[_N_MAX_STATIC_BUFF_LEN];
		if (iWeight > 0 && iWeight < nWeightings) {
			sprintf(szWeightName, "_Weight_%d", iWeight - 1);
		} else {
			sprintf(szWeightName, "%s", "");
		}

		char tempFName[N_MAXFNAMELEN];
		sprintf(tempFName, "%sDPCData%s.txt", szPrefixOutput(), szWeightName);

		//FILE *pDPCFile;
		pDPCFiles[iWeight] = fopen(tempFName, "w");
		if (pDPCFiles[iWeight]) {
			//fclose(pDPCFile);
			//Keep open and use new writing...
			;
		} else {
			printk("\nERROR: Disease Progress curve data unable to create file %s \n\n", tempFName);
		}

		getDPCHeader();

		printToFilePointerAndScreen(pDPCFiles[iWeight], 0, szLineBuffer);

		if (!bBufferDPCOutput) {
			fflush(pDPCFiles[iWeight]);
		}
	}

	pZoningDPCFiles.resize(nZonings);
	for (int iZoning = 0; iZoning < nZonings; ++iZoning) {

		auto publicZonings = SummaryDataManager::getPublicZonings();

		int iSummaryZoning = publicZonings[iZoning];

		int nZonesThisZoning = SummaryDataManager::getNZones(iSummaryZoning);

		pZoningDPCFiles[iZoning].resize(nZonesThisZoning);
		for (int iZoneThisZoning = 0; iZoneThisZoning < nZonesThisZoning; ++iZoneThisZoning) {
			char szZoneName[_N_MAX_STATIC_BUFF_LEN];
			sprintf(szZoneName, "_Zoning_%d_Zone_%d", iSummaryZoning, SummaryDataManager::getZoneID(iSummaryZoning, iZoneThisZoning));

			char tempFName[N_MAXFNAMELEN];
			sprintf(tempFName, "%sDPCData%s.txt", szPrefixOutput(), szZoneName);

			pZoningDPCFiles[iZoning][iZoneThisZoning] = fopen(tempFName, "w");
			if (pZoningDPCFiles[iZoning][iZoneThisZoning]) {
				//fclose(pDPCFile);
				//Keep open and use new writing...
				;
			} else {
				printk("\nERROR: Disease Progress curve data unable to create file %s \n\n", tempFName);
			}

			getDPCHeader();

			printToFilePointerAndScreen(pZoningDPCFiles[iZoning][iZoneThisZoning], 0, szLineBuffer);

			if (!bBufferDPCOutput) {
				fflush(pZoningDPCFiles[iZoning][iZoneThisZoning]);
			}
		}

	}

	


	if (bSampleKernelStatistics) {

		nKernelQuantities = 3;

		dTimeLastOutput = 0.0;

		pdKernelQuantities.resize(DispersalManager::nKernels);

		pKernelDataFiles.resize(DispersalManager::nKernels);

		for (int i = 0; i < DispersalManager::nKernels; i++) {

			//Initialise the quantities storage:
			pdKernelQuantities[i].resize(nKernelQuantities);
			for (int iQ = 0; iQ < nKernelQuantities; iQ++) {
				pdKernelQuantities[i][iQ] = 0.0;
			}

			char fNameTemp[N_MAXFNAMELEN];
			sprintf(fNameTemp, "%sKernel_%d_DataFile.txt", szPrefixOutput(), i);

			pKernelDataFiles[i] = fopen(fNameTemp, "w");

			if (pKernelDataFiles[i]) {

				char *szTempBuffer = szLineBuffer;

				char szItemBuffer[64];

				sprintf(szItemBuffer, "Time");
				szTempBuffer += sprintf(szLineBuffer, "%16s", szItemBuffer);

				sprintf(szItemBuffer, "KernelAttempts");
				szTempBuffer += sprintf(szTempBuffer, "%28s", szItemBuffer);

				sprintf(szItemBuffer, "KernelSuccesses");
				szTempBuffer += sprintf(szTempBuffer, "%28s", szItemBuffer);

				sprintf(szItemBuffer, "KernelExports");
				szTempBuffer += sprintf(szTempBuffer, "%28s", szItemBuffer);

				//Titles:
				fprintf(pKernelDataFiles[i], "%s\n", szLineBuffer);

				//Write first line and keep open for writing:
				szTempBuffer = szLineBuffer;

				//Time at start:
				szTempBuffer += sprintf(szLineBuffer, "%16.8f", 0.0);

				//All zero to begin with:
				for (int iQ = 0; iQ < nKernelQuantities; iQ++) {
					szTempBuffer += sprintf(szTempBuffer, "%20.8f", 0.0);
				}

				fprintf(pKernelDataFiles[i], "%s\n", szLineBuffer);

				if (!bBufferDPCOutput) {
					fflush(pKernelDataFiles[i]);
				}

				//Keep file open for writing...

			} else {
				printk("\nERROR: Disease Progress curve data unable to create file %s\n\n", fNameTemp);
			}

		}
	}

	return 1;
}

int InterOutputDataDPC::reset() {

	if (enabled) {

		/*for(int iWeight=0; iWeight<nWeightings; iWeight++) {

			getDPCLine(world->timeEnd, iWeight);

			printToFilePointerAndScreen(pDPCFiles[iWeight], 0, "%s", szLineBuffer);

		}*/

		setupOutput();

		timeNext = timeFirst;

	}

	return enabled;

}

double InterOutputDataDPC::getFrequency() {
	return frequency;
}

int InterOutputDataDPC::intervene() {
	//DPC data

	//Avoid double final line output by preventing DPC from writing within 1% output fq before end of simulation
	if (world->timeEnd - timeNext > 0.01*frequency) {

		for (int iWeight = 0; iWeight < nWeightings; iWeight++) {

			getDPCLine(timeNext, iWeight);

			//Why !bQuiet doesnt work I dont know
			int shouldPrintToScreen = 1;
			if (bQuiet || iWeight > 0) {
				shouldPrintToScreen = 0;
			}
			printToFilePointerAndScreen(pDPCFiles[iWeight], shouldPrintToScreen, "%s", szLineBuffer);

			if (!bBufferDPCOutput) {
				fflush(pDPCFiles[iWeight]);
				if (shouldPrintToScreen) {
					fflush(stdout);
				}
			}
		}

		for (int iZoning = 0; iZoning < nZonings; ++iZoning) {

			auto publicZonings = SummaryDataManager::getPublicZonings();

			int iSummaryZoning = publicZonings[iZoning];

			int nZonesThisZoning = SummaryDataManager::getNZones(iSummaryZoning);

			for (int iZoneThisZoning = 0; iZoneThisZoning < nZonesThisZoning; ++iZoneThisZoning) {
				
				getDPCLine(timeNext, -1, iZoning, iZoneThisZoning);

				printToFilePointerAndScreen(pZoningDPCFiles[iZoning][iZoneThisZoning], 0, "%s", szLineBuffer);

				if (!bBufferDPCOutput) {
					fflush(pZoningDPCFiles[iZoning][iZoneThisZoning]);
				}
			}

		}


		if (bSampleKernelStatistics) {
			writeKernelStats(timeNext);
		}

	}

	timeNext += frequency;

	return 1;
}

int InterOutputDataDPC::writeKernelStats(double dTimeNow) {

	for (int i = 0; i < DispersalManager::nKernels; i++) {
		//Output marginal increases in each quantity (normalised relative to nMaxHosts):
		double dCurrentAttempts = double(DispersalManager::getNKernelLongAttempts(i));
		double dCurrentSuccesses = double(DispersalManager::getNKernelLongSuccess(i));
		double dCurrentExports = double(DispersalManager::getNKernelLongExport(i));


		vector<double> &pQuantities = pdKernelQuantities[i];

		char *szTempBuffer = szLineBuffer;

		//Time:
		szTempBuffer += sprintf(szLineBuffer, "%16.8f", dTimeNow);

		//Enter new normalised marginal contributions:
		szTempBuffer += sprintf(szTempBuffer, "%20.8f", (double(dCurrentAttempts) - pQuantities[0]) / double(PopulationCharacteristics::nMaxHosts));
		szTempBuffer += sprintf(szTempBuffer, "%20.8f", (double(dCurrentSuccesses) - pQuantities[1]) / double(PopulationCharacteristics::nMaxHosts));
		szTempBuffer += sprintf(szTempBuffer, "%20.8f", (double(dCurrentExports) - pQuantities[2]) / double(PopulationCharacteristics::nMaxHosts));

		//Write to file:
		fprintf(pKernelDataFiles[i], "%s\n", szLineBuffer);

		//Update stored values to current values:
		pQuantities[0] = dCurrentAttempts;
		pQuantities[1] = dCurrentSuccesses;
		pQuantities[2] = dCurrentExports;

		if (!bBufferDPCOutput) {
			fflush(pKernelDataFiles[i]);
		}
	}


	return 1;
}

int InterOutputDataDPC::finalAction() {
	//output the final DPC data
	//printf("DataDPC::finalAction()\n");

	if (enabled) {

		for (int iWeight = 0; iWeight < nWeightings; iWeight++) {

			getDPCLine(world->timeEnd, iWeight);

			printToFilePointerAndScreen(pDPCFiles[iWeight], 0, "%s", szLineBuffer);
			
			if (!bBufferDPCOutput) {
				fflush(pDPCFiles[iWeight]);
			}
		}

		for (int iZoning = 0; iZoning < nZonings; ++iZoning) {

			auto publicZonings = SummaryDataManager::getPublicZonings();

			int iSummaryZoning = publicZonings[iZoning];

			int nZonesThisZoning = SummaryDataManager::getNZones(iSummaryZoning);

			for (int iZoneThisZoning = 0; iZoneThisZoning < nZonesThisZoning; ++iZoneThisZoning) {

				getDPCLine(world->timeEnd, -1, iZoning, iZoneThisZoning);

				printToFilePointerAndScreen(pZoningDPCFiles[iZoning][iZoneThisZoning], 0, "%s", szLineBuffer);

				if (!bBufferDPCOutput) {
					fflush(pZoningDPCFiles[iZoning][iZoneThisZoning]);
				}
			}

		}

	}

	if (bSampleKernelStatistics) {
		writeKernelStats(world->timeEnd);
	}

	return 1;
}

void InterOutputDataDPC::writeSummaryData(FILE *pFile) {
	return;
}

int InterOutputDataDPC::getDPCHeader() {

	char *szTempBuffer = szLineBuffer;

	char szItemBuffer[64];

	sprintf(szItemBuffer, "Time");
	szTempBuffer += sprintf(szLineBuffer, "%12s", szItemBuffer);

	for (int i = 0; i < PopulationCharacteristics::nPopulations; i++) {
		for (int j = 0; j < Population::pGlobalPopStats[i]->nTotalClasses; j++) {
			sprintf(szItemBuffer, "%s_%d", Population::pGlobalPopStats[i]->pszClassNames[j], i);
			szTempBuffer += sprintf(szTempBuffer, "%16s", szItemBuffer);
		}
		sprintf(szItemBuffer, "nPopulationsSusceptible_%d", i);
		szTempBuffer += sprintf(szTempBuffer, "%28s", szItemBuffer);
		sprintf(szItemBuffer, "nPopulationsInfected_%d", i);
		szTempBuffer += sprintf(szTempBuffer, "%28s", szItemBuffer);
		sprintf(szItemBuffer, "nPopulationsEverInfected_%d", i);
		szTempBuffer += sprintf(szTempBuffer, "%28s", szItemBuffer);
	}

	//Always give location metrics even when have only one population, for consistency:
	sprintf(szItemBuffer, "nLocationsSusceptible");
	szTempBuffer += sprintf(szTempBuffer, "%24s", szItemBuffer);
	sprintf(szItemBuffer, "nLocationsInfected");
	szTempBuffer += sprintf(szTempBuffer, "%24s", szItemBuffer);
	sprintf(szItemBuffer, "nLocationsEverInfected");
	szTempBuffer += sprintf(szTempBuffer, "%24s", szItemBuffer);

	szTempBuffer += sprintf(szTempBuffer, "\n");

	return 1;
}

int InterOutputDataDPC::getDPCLine(double dTimeNow, int iWeighting, int iZoning, int iZone) {

	if (iWeighting >= 0 && (iZoning >= 0 || iZone >= 0)) {
		fprintf(stderr, "ERROR: Requesting weighting and zones simultaneously\n");
	}

	char *szTempBuffer = szLineBuffer;

	szTempBuffer += sprintf(szLineBuffer, "%12.6f", dTimeNow);

	for (int i = 0; i < PopulationCharacteristics::nPopulations; i++) {
		vector<double> pdTempHostAlloc;
		if (iWeighting >= 0) {
			pdTempHostAlloc = SummaryDataManager::getHostQuantities(i, iWeighting);
		} else {
			pdTempHostAlloc = SummaryDataManager::getHostQuantitiesZone(i, iZoning, iZone);
		}

		for (int j = 0; j < Population::pGlobalPopStats[i]->nTotalClasses; j++) {
			double dVal = pdTempHostAlloc[j];
			//Ensure that values that are infinitesimally negative due to machine rounding errors do not have a minus sign:
			if (dVal < 0.0) {
				dVal = 0.0;
			}
			szTempBuffer += sprintf(szTempBuffer, "%16.6f", dVal);
		}

		double nPopSus, nPopInf;
		if (iWeighting >= 0) {
			nPopSus = SummaryDataManager::getCurrentPopulationsSusceptible(iWeighting)[i];
			nPopInf = SummaryDataManager::getCurrentPopulationsInfected(iWeighting)[i];
		} else {
			nPopSus = SummaryDataManager::getCurrentPopulationsSusceptibleZone(iZoning, iZone)[i];
			nPopInf = SummaryDataManager::getCurrentPopulationsInfectedZone(iZoning, iZone)[i];
		}

		szTempBuffer += sprintf(szTempBuffer, "%28.6f", nPopSus);//nPop Sus
		szTempBuffer += sprintf(szTempBuffer, "%28.6f", nPopInf);//nPop Inf

		double nUniquePopInf;
		if (iWeighting >= 0) {
			nUniquePopInf = SummaryDataManager::getUniquePopulationsInfected(iWeighting)[i];
		} else {
			nUniquePopInf = SummaryDataManager::getUniquePopulationsInfectedZone(iZoning, iZone)[i];
		}
		
		szTempBuffer += sprintf(szTempBuffer, "%28.6f", nUniquePopInf);//nPop EverInf

	}

	//Always give location metrics, even when only have one population, for consistency:
	double nLocSus, nLocInf;
	if (iWeighting >= 0) {
		nLocSus = SummaryDataManager::getCurrentLocationsSusceptible(iWeighting);
		nLocInf = SummaryDataManager::getCurrentLocationsInfected(iWeighting);
	} else {
		nLocSus = SummaryDataManager::getCurrentLocationsSusceptibleZone(iZoning, iZone);
		nLocInf = SummaryDataManager::getCurrentLocationsInfectedZone(iZoning, iZone);
	}

	szTempBuffer += sprintf(szTempBuffer, "%24.6f", nLocSus);//nPop Sus
	szTempBuffer += sprintf(szTempBuffer, "%24.6f", nLocInf);//nPop Inf

	double nUniqueLocInf;
	if (iWeighting >= 0) {
		nUniqueLocInf = SummaryDataManager::getUniqueLocationsInfected(iWeighting);
	} else {
		nUniqueLocInf = SummaryDataManager::getUniqueLocationsInfectedZone(iZoning, iZone);
	}

	szTempBuffer += sprintf(szTempBuffer, "%24.6f", nUniqueLocInf);//nPop EverInf

	szTempBuffer += sprintf(szTempBuffer, "\n");

	return 1;
}

void InterOutputDataDPC::displayHeaderLine() {

	if (enabled) {
		getDPCHeader();
		if (!bQuiet) {
			printk(szLineBuffer);
		}
	}

	return;
}
