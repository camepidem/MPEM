//InterEndSim.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "PopulationCharacteristics.h"
#include "RateManager.h"
#include "DispersalManager.h"
#include "SummaryDataManager.h"
#include "InterSpatialStatistics.h"
#include "SpatialStatistic.h"
#include "InterBatch.h"
#include "SimulationProfiler.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "InterEndSim.h"
#include <chrono>

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

void wallclockAbortFunction(int timeLimit, Landscape *target) {

	std::this_thread::sleep_for(std::chrono::seconds(timeLimit));

	fprintf(stderr, "Simulation has reached specified wallclock time limit of %d seconds, aborting and saving data.", timeLimit);

	target->abortSimulation(1);
}

InterEndSim::InterEndSim() {
	timeSpent = 0;
	InterName = "EndSimulation";
	
	enabled = 1;
	bEndAtTimeCondition = true;
	frequency = (world->timeEnd - world->timeStart);

	setCurrentWorkingSubection(InterName, CM_SIM);

	//Read the ending conditions file
	char szEndConditionsFileName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szEndConditionsFileName, "EndSimulationConditions.txt");
	int bEndConditionsFile = readValueToProperty("EndSimulationConditionsFileName", szEndConditionsFileName, -1, "#FileName#");

	int wallclockTimeLimit = -1;
	int bUseWallclockAbort = readValueToProperty("EndSimulationAbortAfterWallclockTimeInSeconds", &wallclockTimeLimit, -1, ">0");
	//TODO: Choose sensible minimum allowed time limit, like 5 minutes?
	if (bUseWallclockAbort && wallclockTimeLimit < 1) {
		reportReadError("ERROR: Specified EndSimulationAbortAfterWallclockTimeInSeconds (%d) is less than minimum allowed value: 1", wallclockTimeLimit);
	}

	if (!world->bGiveKeys) {

		if (bEndConditionsFile) {
			conditions = readConditionsFromFile(szEndConditionsFileName);
		}

		if (bUseWallclockAbort) {
			printk("Wallclock abort enabled. Simulation will terminate and write full state information if not complete within %d seconds.\n", wallclockTimeLimit);
			wallclockAbortThread = std::thread(wallclockAbortFunction, wallclockTimeLimit, world);
			wallclockAbortThread.detach();
		}

	}
	

	timeFirst = findNextTime(world->timeStart - 1.0);
	timeNext = timeFirst;

}

InterEndSim::~InterEndSim() {
	//do nothing
}

bool InterEndSim::disableEndAtTimeCondition() {
	bEndAtTimeCondition = false;
	return false;
}

int InterEndSim::intervene() {

	if (enabled) {

		int nEndingConditionsReached = 0;

		//Time ending condition:
		if (bEndAtTimeCondition && timeNext >= world->timeEnd) {
			nEndingConditionsReached++;
		}

		if (nEndingConditionsReached == 0) {//Don't bother checking anything else if we're already stopping:

			for (auto condition : conditions) {
				if (condition.time == timeNext) {
					if (testConstraint(condition)) {
						nEndingConditionsReached++;
						break;
					}
				}
			}

		}

		if (nEndingConditionsReached > 0) {
			if (!world->interventionManager->pInterBatch->enabled || world->interventionManager->pInterBatch->bBatchDone) {
				//Actually end the simulation:
				world->run = 0;
				world->timeEnd = timeNext;
				printk("Simulation Ended\n");
			} else {
				//Still in a batch, so just restart:
				world->interventionManager->pInterBatch->performIntervention();
			}
		}

		timeNext = findNextTime(timeNext);
	}

	return enabled;
}

double InterEndSim::getTimeNext() {
	if(enabled) {
		return timeNext;
	} else {
		return world->timeEnd + 1e30;
	}
}

int InterEndSim::setTimeNext(double dTimeNext) {
	timeNext = dTimeNext;

	return 1;
}

bool InterEndSim::bEarlyStoppingConditionsEnabled() {
	return conditions.size() > 0;
}

int InterEndSim::finalAction() {
	//call final data output routine
	//printf("Producing Simulation Summary\n");
	FILE *pSimFile;
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char *timestring = asctime(timeinfo);

	char fName[256];
	sprintf(fName,"%sSimulationDataSummary.txt",szPrefixOutput());

	pSimFile = fopen(fName,"w");
	if(pSimFile) {

		printHeaderText(pSimFile,"Simulation Data Summary:");

		fprintf(pSimFile,"Version: %s\n\n",getVersionString());

		fprintf(pSimFile,"Run: %s",timestring);

		//Simulation Data:
		printHeaderText(pSimFile,"Simulation Parameters:");

		fprintf(pSimFile,"RNG Seed                %lu\n",world->RNGSeed);
		fprintf(pSimFile,"SimulationStartTime %-12.6f\n", world->timeStart);
		fprintf(pSimFile,"SimulationLength    %-12.6f\n", (world->timeEnd - world->timeStart));
		fprintf(pSimFile,"MaxHosts                %d\n",PopulationCharacteristics::nMaxHosts);
		fprintf(pSimFile,"Model                   %s\n",world->szFullModelType);
		
		//Rate Data:
		RateManager::writeSummaryData(pSimFile);

		fclose(pSimFile);

		//Kernel Data:
		DispersalManager::writeSummaryData(fName);

		//Profiling Data:
		world->profiler->writeSummaryData(fName, DispersalManager::getNKernelLongAttempts(-1), DispersalManager::getNKernelLongSuccess(-1));

		//Intervention Data:
		world->interventionManager->writeSummaryData(fName);

	} else {
		printk("\nERROR: Simulation unable to write summary file\n\n");
	}
	return 1;
}

std::vector<EndSimCondition> InterEndSim::readConditionsFromFile(std::string fileName) {

	std::vector<EndSimCondition> readConditions;

	//Get the conditions from file:
	std::vector<std::string> header = {"Time", "Statistic", "Metric", "TargetData", "Comparator", "Value"};
	
	std::ifstream inFile(fileName);

	//Header:
	std::string headerLine;
	if (!std::getline(inFile, headerLine)) {
		reportReadError("ERROR: Unable to read header from End simulations conditions file %s\n", fileName.c_str());
		return {};
	}

	std::vector<std::string> fileHeader;
	split(fileHeader, headerLine, ",");
	for (auto &headerElement : fileHeader) {
		trim(headerElement);//Remove leading/trailing whitespace for consistency
		stringToUpper(headerElement);//Convert to uppercase for consistent comparison
	}

	//Check that all necessary header elements are present:
	for(auto headerElement : header) {
		auto testStr = headerElement;
		stringToUpper(testStr);
		if (std::find(fileHeader.begin(), fileHeader.end(), testStr) == fileHeader.end()) {
			reportReadError("ERROR: Unable to find required header element \"%s\" in End simulation conditions file %s\n", headerElement.c_str(), fileName.c_str());
			return {};
		}
	}

	//Actually read the data:
	std::string line;
	while (std::getline(inFile, line)) {
		std::vector<std::string> rowElements;
		split(rowElements, line, ",");

		EndSimCondition newConstraint;

		newConstraint.time = std::stod(rowElements[std::find(fileHeader.begin(), fileHeader.end(), "TIME") - fileHeader.begin()]);


		newConstraint.statistic = rowElements[std::find(fileHeader.begin(), fileHeader.end(), "STATISTIC") - fileHeader.begin()];
		trim(newConstraint.statistic);
		stringToUpper(newConstraint.statistic);

		//TODO: Move these checks to using the same code that processes the constraints
		std::vector<std::string> validStatistics = { "DPC", "SPATIALSTATISTICSCOUNT", "SPATIALSTATISTICSRING", "SPATIALSTATISTICSMORANI", "SPATIALSTATISTICSCLUSTER" };

		if (std::find(validStatistics.begin(), validStatistics.end(), newConstraint.statistic) == validStatistics.end()) {
			reportReadError("ERROR: End simulation constraint specified invalid statistic: \"%s\" on line %d of file %s", newConstraint.statistic.c_str(), readConditions.size(), fileName.c_str());
			reportReadError("Valid statistics:\n");
			for (auto stat : validStatistics) {
				reportReadError("%s ", stat.c_str());
			}
			reportReadError("\n");
			return {};
		}


		newConstraint.metric = rowElements[std::find(fileHeader.begin(), fileHeader.end(), "METRIC") - fileHeader.begin()];
		trim(newConstraint.metric);
		stringToUpper(newConstraint.metric);
		//TODO: Hard to verify this ahead of time


		newConstraint.targetData = rowElements[std::find(fileHeader.begin(), fileHeader.end(), "TARGETDATA") - fileHeader.begin()];
		trim(newConstraint.targetData);
		if (newConstraint.statistic == "DPC") {
			stringToUpper(newConstraint.targetData);//Don't do this in the case of spatial statistics, as it will represent a file
		} else {

			//Have we already loaded this file?
			if (targetData.find(newConstraint.targetData) == targetData.end()) {

				Raster<int> targetRaster;
				if (!targetRaster.init(newConstraint.targetData)) {
					reportReadError("ERROR: End simulation constraint specified invalid target data raster file: \"%s\" on line %d of file %s", newConstraint.targetData.c_str(), readConditions.size(), fileName.c_str());
					return {};
				}

				targetData[newConstraint.targetData] = new TargetData(newConstraint.targetData + "_ENDSIM", targetRaster);//TODO: Adding _ENDSIM here may be counterproductive, in that the name of the file is incredibly unlikely to collide with an ID created from a source that is not a filename, of which there are currently none...

			}

		}


		newConstraint.comparator = rowElements[std::find(fileHeader.begin(), fileHeader.end(), "COMPARATOR") - fileHeader.begin()];
		trim(newConstraint.comparator);

		std::vector<std::string> validComparators = { "==", "!=", ">", "<", ">=", "<=" };

		if (std::find(validComparators.begin(), validComparators.end(), newConstraint.comparator) == validComparators.end()) {
			reportReadError("ERROR: End simulation constraint specified invalid comparator: \"%s\" on line %d of file %s", newConstraint.comparator.c_str(), readConditions.size(), fileName.c_str());
			reportReadError("Valid Comparators:\n");
			for (auto cmp : validComparators) {
				reportReadError("%s ", cmp.c_str());
			}
			reportReadError("\n");
			return {};
		}


		newConstraint.value = std::stod(rowElements[std::find(fileHeader.begin(), fileHeader.end(), "VALUE") - fileHeader.begin()]);


		readConditions.push_back(newConstraint);

	}


	//Sort them to ensure they are in time order:
	std::sort(readConditions.begin(), readConditions.end(), [](const EndSimCondition &a, const EndSimCondition &b) {return a.time < b.time; });

	return readConditions;
}

bool InterEndSim::testConstraint(EndSimCondition constraint) {

	//First get the value of the statistic:
	double statValue;

	if (constraint.statistic == "DPC") {

		enum DPCDataAggregation { ByPop, ByLoc };
		DPCDataAggregation dpcDataAggregation;
		int iPop;
		enum DPCDataType { HostClass, N_Sus, N_Inf, N_EverInf };
		DPCDataType dpcDataType;
		int iClass;

		//All the location/population measures e.g. nLocationsInfected start with N, and none of the classes start with an N
		if (constraint.metric[0] == 'N') {

			bool parseProblems = false;

			if (constraint.metric.find("SUSCEPTIBLE")) {
				dpcDataType = N_Sus;
			} else if (constraint.metric.find("EVERINFECTED")) {
				dpcDataType = N_EverInf;
			} else if (constraint.metric.find("INFECTED")) {
				dpcDataType = N_Inf;
			} else {
				parseProblems = true;
			}


			if (constraint.metric.find("POPULATIONS")) {

				dpcDataAggregation = ByPop;

				std::vector<std::string> popDataSpl;
				split(popDataSpl, constraint.metric, "_");

				if (popDataSpl.size() != 2) {
					parseProblems = true;
				} else {
					iPop = std::stoi(popDataSpl[1]);
				}

			} else {
				if (constraint.metric.find("LOCATIONS")) {
					dpcDataAggregation = ByLoc;
				} else {
					parseProblems = true;
				}
			}

			if (parseProblems) {
				reportReadError("ERROR: End Sim constraints: Unable to parse DPC data measure and population/location information from Metric=\"%s\"\n", constraint.metric.c_str());
				reportReadError("Valid values are of the form\n");
				reportReadError("nLocations<Measure>\n");
				reportReadError("or\n");
				reportReadError("nPopulations<Measure>_#NPOP\n");
				reportReadError("Where <Measure> is one of {Susceptible, Infected, EverInfected}\n");
				return false;
			}

		} else {

			dpcDataAggregation = ByPop;
			dpcDataType = HostClass;

			std::vector<std::string> popClassSpl;
			split(popClassSpl, constraint.metric, "_");

			if (popClassSpl.size() != 2) {
				reportReadError("ERROR: End Sim constraints: Unable to parse DPC class name and population index from Metric=\"%s\"\n", constraint.metric.c_str());
				reportReadError("Valid class names are of the form\n");
				reportReadError("CLASSNAME_#POP\n");
				reportReadError("Example: SUSCEPTIBLE_0\n");
				return false;
			}

			iPop = std::stoi(popClassSpl[1]);

			if (iPop < 0 || iPop >= PopulationCharacteristics::nPopulations) {
				reportReadError("ERROR: End Sim constraints: Invalid population \"%s\" specified in Metric=\"%s\". Valid population indices are [0, %d]\n", popClassSpl[1].c_str(), constraint.metric.c_str(), PopulationCharacteristics::nPopulations - 1);
				return false;
			}

			iClass = -1;
			for (int iPopClass = 0; iPopClass < Population::pGlobalPopStats[iPop]->nClasses; ++iPopClass) {
				if (popClassSpl[0] == Population::pGlobalPopStats[iPop]->pszClassNames[iPopClass]) {
					iClass = iPopClass;
					break;
				}
			}

			if (iClass == -1) {
				reportReadError("ERROR: End Sim constraints: Invalid class name \"%s\" specified in Metric=\"%s\". Valid classes for population %d are:\n", popClassSpl[0].c_str(), constraint.metric.c_str(), PopulationCharacteristics::nPopulations - 1);
				for (int iPopClass = 0; iPopClass < Population::pGlobalPopStats[iPop]->nClasses; ++iPopClass) {
					reportReadError("%s\n", Population::pGlobalPopStats[iPop]->pszClassNames[iPopClass]);
				}
				return false;
			}

		}

		bool weightZoneParseSuccess = false;

		//Normal:
		int iWeight = 0;
		if (constraint.targetData == "") {
			weightZoneParseSuccess = true;
		}

		std::vector<std::string> weightZoneSpl;
		split(weightZoneSpl, constraint.targetData, "_");

		//Weighted:
		if (weightZoneSpl.size() == 2 && weightZoneSpl[0] == "WEIGHT") {

			iWeight = std::stoi(weightZoneSpl[1]);

			int nWeightings = SummaryDataManager::getNWeightings();
			if (iWeight < 0 || iWeight >= nWeightings) {
				reportReadError("ERROR: End Sim constraints: Invalid weighting index \"%s\" specified in TargetData=\"%s\". ", weightZoneSpl[1].c_str(), constraint.targetData.c_str());
				if (nWeightings > 1) {
					reportReadError("Valid weighting indices are[0, %d]\n", nWeightings - 1);
				} else {
					reportReadError("DPC Weightings are not enabled\n");
				}
				return false;
			}

			iWeight++;//Weights are stored such that internally weight zero is the normal DPCs, so what is reported as WEIGHT_0 is in fact internally iWeight=1 and so on (bleugh)

			weightZoneParseSuccess = true;
		}

		if (weightZoneParseSuccess) {

			switch (dpcDataType) {
				case DPCDataType::HostClass:
					statValue = SummaryDataManager::getHostQuantities(iPop, iWeight)[iClass];
					break;
				case DPCDataType::N_Sus:
					if (dpcDataAggregation == DPCDataAggregation::ByPop) {
						statValue = SummaryDataManager::getCurrentPopulationsSusceptible(iWeight)[iPop];
					} else {
						statValue = SummaryDataManager::getCurrentLocationsSusceptible(iWeight);
					}
					break;
				case DPCDataType::N_Inf:
					if (dpcDataAggregation == DPCDataAggregation::ByPop) {
						statValue = SummaryDataManager::getCurrentPopulationsInfected(iWeight)[iPop];
					} else {
						statValue = SummaryDataManager::getCurrentLocationsInfected(iWeight);
					}
					break;
				case DPCDataType::N_EverInf:
					if (dpcDataAggregation == DPCDataAggregation::ByPop) {
						statValue = SummaryDataManager::getUniquePopulationsInfected(iWeight)[iPop];
					} else {
						statValue = SummaryDataManager::getUniqueLocationsInfected(iWeight);
					}
					break;
			}

		}

		//Zoning:
		if (weightZoneSpl.size() == 4 && weightZoneSpl[0] == "ZONING") {

			int iZoning = std::stoi(weightZoneSpl[1]);

			auto allZonings = SummaryDataManager::getPublicZonings();

			if (std::find(allZonings.begin(), allZonings.end(), iZoning) == allZonings.end()) {
				reportReadError("ERROR: End Sim constraints: Invalid zoning index \"%s\" specified in TargetData=\"%s\". ", weightZoneSpl[1].c_str(), constraint.targetData.c_str());
				if (allZonings.size() > 0) {
					reportReadError("Valid zoning indices are : \n");
					for (int aZoning : allZonings) {
						reportReadError("%d ", aZoning);
					}
					reportReadError("\n");
				} else {
					reportReadError("There are no zonings enabled\n");
				}

				return false;
			}


			int iZone = std::stoi(weightZoneSpl[3]);

			int nZonesThisZoning = SummaryDataManager::getNZones(iZoning);

			int iZoneID;
			bool bExternalZoningFound = false;
			for (int iInternalZone = 0; iInternalZone < nZonesThisZoning; ++iInternalZone) {
				int iExternalZone = SummaryDataManager::getZoneID(iZoning, iInternalZone);
				if (iZone == iExternalZone) {
					iZoneID = iExternalZone;
					bExternalZoningFound = true;
					break;
				}
			}

			if (!bExternalZoningFound) {
				reportReadError("ERROR: End Sim constraints: Invalid zone index \"%s\" for zone %d specified in TargetData=\"%s\". Valid zone indices are:\n", weightZoneSpl[3].c_str(), iZone, constraint.targetData.c_str());
				for (int iInternalZone = 0; iInternalZone < nZonesThisZoning; ++iInternalZone) {
					int iExternalZone = SummaryDataManager::getZoneID(iZoning, iInternalZone);
					reportReadError("%d ", iExternalZone);
				}
				reportReadError("\n");
				return false;
			}

			statValue = SummaryDataManager::getHostQuantitiesZone(iPop, iZoning, iZone)[iClass];

			//TODO: Duplication nightmare
			switch (dpcDataType) {
				case DPCDataType::HostClass:
					statValue = SummaryDataManager::getHostQuantitiesZone(iPop, iZoning, iZone)[iClass];;
					break;
				case DPCDataType::N_Sus:
					if (dpcDataAggregation == DPCDataAggregation::ByPop) {
						statValue = SummaryDataManager::getCurrentPopulationsSusceptibleZone(iZoning, iZone)[iPop];
					} else {
						statValue = SummaryDataManager::getCurrentLocationsSusceptibleZone(iZoning, iZone);
					}
					break;
				case DPCDataType::N_Inf:
					if (dpcDataAggregation == DPCDataAggregation::ByPop) {
						statValue = SummaryDataManager::getCurrentPopulationsInfectedZone(iZoning, iZone)[iPop];
					} else {
						statValue = SummaryDataManager::getCurrentLocationsInfectedZone(iZoning, iZone);
					}
					break;
				case DPCDataType::N_EverInf:
					if (dpcDataAggregation == DPCDataAggregation::ByPop) {
						statValue = SummaryDataManager::getUniquePopulationsInfectedZone(iZoning, iZone)[iPop];
					} else {
						statValue = SummaryDataManager::getUniqueLocationsInfectedZone(iZoning, iZone);
					}
					break;
			}

			weightZoneParseSuccess = true;
		}


		if (!weightZoneParseSuccess) {
			reportReadError("ERROR: End Sim constraints: Unable to parse DPC weight or zoning information from TargetData=\"%s\"\n", constraint.targetData.c_str());
			reportReadError("Valid weight/zoning values are of the form\n");
			reportReadError("WEIGHT_N\n");
			reportReadError("ZONING_N_ZONE_M\n");
			reportReadError("or an empty string for normal DPC data.\n");
			return false;
		}

	} else {

		InterSpatialStatistics::SpatialStatisticTypes targetSpatialStatistic;

		if (constraint.statistic == "SPATIALSTATISTICSCOUNT") {//TODO: store this in a map, then can use that one map for both validation and here
			targetSpatialStatistic = InterSpatialStatistics::SpatialStatisticTypes::SS_COUNTS;
		}

		if (constraint.statistic == "SPATIALSTATISTICSRING") {
			targetSpatialStatistic = InterSpatialStatistics::SpatialStatisticTypes::SS_RING;
		}

		if (constraint.statistic == "SPATIALSTATISTICSMORANI") {
			targetSpatialStatistic = InterSpatialStatistics::SpatialStatisticTypes::SS_MORANI;
		}

		if (constraint.statistic == "SPATIALSTATISTICSCLUSTER") {
			targetSpatialStatistic = InterSpatialStatistics::SpatialStatisticTypes::SS_CLUSTER;
		}

		auto fullStats = world->interventionManager->pInterSpatialStatistics->calculateStats(constraint.time, *targetData[constraint.targetData], targetSpatialStatistic);

		if (fullStats.size() == 0) {
			reportReadError("ERROR: End Sim constraints: Spatial Statistic %s was not enabled. Spatial statistics must be enabled and configured to be used in End Sim constraints.\n", constraint.statistic.c_str());
			return false;
		}

		if (fullStats.find(constraint.metric) == fullStats.end()) {
			reportReadError("ERROR: End Sim constraints: Invalid metric name \"%s\" for Statistic %s\n", constraint.metric.c_str(), constraint.statistic.c_str());
			return false;
		}

		statValue = fullStats[constraint.metric];
	}

	//Now compare value of statistic to the target value via the specified comparator:
	if (constraint.comparator == "==") {//TODO: store this in a map from string to classes that provide overloads for a cmp function, then can use that one map for both validation and here
		return statValue == constraint.value;
	}

	if (constraint.comparator == "!=") {
		return statValue != constraint.value;
	}

	if (constraint.comparator == ">=") {
		return statValue >= constraint.value;
	}

	if (constraint.comparator == "<=") {
		return statValue <= constraint.value;
	}

	if (constraint.comparator == ">") {
		return statValue > constraint.value;
	}

	if (constraint.comparator == "<") {
		return statValue < constraint.value;
	}

	reportReadError("ERROR: EndSim constraints: Invalid comparator \"%s\" slipped through validation to live use.\n", constraint.comparator.c_str());
	return false;
}

double InterEndSim::findNextTime(double dCurrentTime) {

	//Return least time greater than dCurrentTime from conditions[].time
	auto lub = std::upper_bound(conditions.begin(), conditions.end(), dCurrentTime, [](double a, const EndSimCondition &b) {return a < b.time; });

	if (lub != conditions.end()) {
		return lub->time;
	}

	//Otherwise,
	if (bEndAtTimeCondition) {
		return world->timeEnd;
	}

	//To prevent endless looping at timeEnd
	return dCurrentTime + 1e30;
}
