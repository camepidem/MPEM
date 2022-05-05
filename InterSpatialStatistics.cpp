//InterSpatialStatistics.cpp

#include "stdafx.h"
#include "myRand.h"
#include "json/json.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "Raster.h"
#include "SpatialStatisticCounts.h"
#include "SpatialStatisticRing.h"
#include "SpatialStatisticMoranI.h"
#include "SpatialStatisticCluster.h"
#include "InterManagement.h"
#include "InterSpatialStatistics.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterSpatialStatistics::InterSpatialStatistics() {

	timeSpent = 0;
	InterName = "SpatialStatistics";

	//Memory housekeeping:
	singleTargetData	= NULL;
	simDataRaster		= NULL;

	setCurrentWorkingSubection(InterName, CM_SIM);

	char szKey[N_MAXSTRLEN];

	enabled = 0;

	sprintf(szKey,"%s%s",InterName, "Enable");

	readValueToProperty(szKey, &enabled, -1, "[0,1]");

	if(enabled) {

		//Read Data:

		printToKeyFile("//Specify EITHER *only* TimesAndFiles OR *all* of Frequency TimeFirst TargetDataFileName\n");

		//Multiple target data files:
		Json::Reader reader;
		Json::Value root;

		char szConfigStuff[_N_MAX_DYNAMIC_BUFF_LEN];
		sprintf(szConfigStuff, "{\"1.0\" : \"myTargetDataRasterName.txt\"}");
		int bUsingTimesAndFiles = readValueToProperty("SpatialStatisticsTimesAndFiles", szConfigStuff, -2, "#JSON{f:#FileName#}#");
		if (bUsingTimesAndFiles && !bIsKeyMode()) {
			if (!reader.parse(szConfigStuff, root)) {
				reportReadError("ERROR: Unable to parse json string (you may need to put double quotes around the times, also note that single quotes are not valid JSON): %s, message: %s\n", szConfigStuff, reader.getFormattedErrorMessages().c_str());
				return;
			}

			if (!root.isObject()) {
				
				if (root.isString()) {
					//TODO: Could do a last chance parse here but could pathologically need to recursively parse forever
					reportReadError("ERROR: SpatialStatisticsTimesAndFiles was a JSON string (%s), not a JSON object (perhaps don't enclose the whole thing in double quotes?)\n", szConfigStuff);
					return;
				}

				reportReadError("ERROR: Unable to parse JSON to JSON object: %s\n", szConfigStuff);
				return;
			}
			
		}

		//Single target data file:
		int bUsingSingleFile = 0;
		frequency = 1.0;
		int bSpecifiedFrequency = readValueToProperty("SpatialStatisticsFrequency", &frequency, -2, ">0");
		bUsingSingleFile += bSpecifiedFrequency;
		if(bSpecifiedFrequency) {
			if(frequency <= 0.0) {
				reportReadError("ERROR: Spatial Statistics frequency negative\n");
			}
		}

		timeFirst = world->timeStart + frequency;
		int bSpecifiedTimeFirst = readValueToProperty("SpatialStatisticsTimeFirst", &timeFirst, -3, ">=0");
		bUsingSingleFile += bSpecifiedTimeFirst;
		if(bSpecifiedTimeFirst) {
			if(timeFirst < world->timeStart) {
				int nPeriodsBehind = ceil((world->timeStart - timeFirst) / frequency);
				double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

				printk("Warning: SpatialStatistics specified first time %f, which is prior to simulation start time (%f).\nAdvancing first time to earliest time >= simulation start time by advancing start time %d periods to %f.\n", timeFirst, world->timeStart, nPeriodsBehind, newTimeFirst);

				timeFirst = newTimeFirst;
			}
		}

		//Target Data file:
		sprintf(szKey,"%s%s",InterName, "TargetDataFileName");
		char targetFileName[_N_MAX_STATIC_BUFF_LEN];
		sprintf(targetFileName, "%s%s%s", world->szPrefixLandscapeRaster, "SpatialStatisticsTargetData", world->szSuffixTextFile);
		int bSpecifiedTargetFileName = readValueToProperty(szKey, targetFileName, -2, "#FileName#");
		bUsingSingleFile += bSpecifiedTargetFileName;

		//How to classify a location as positive:
		pcPositiveCriterion = PC_SYMPTOMATIC;
		int nCriteriaRead = 0;
		int bCurrentCriterion;

		bCurrentCriterion = 1;
		sprintf(szKey,"%s%s",InterName, "PositiveCriterionSymptomatic");
		if(readValueToProperty(szKey, &bCurrentCriterion, -2, "[0,1]")) {
			if(bCurrentCriterion) {
				nCriteriaRead++;
				pcPositiveCriterion = PC_SYMPTOMATIC;
			}
		}

		bCurrentCriterion = 0;
		sprintf(szKey,"%s%s",InterName, "PositiveCriterionSeverity");
		if(readValueToProperty(szKey, &bCurrentCriterion, -2, "[0,1]")) {
			if(bCurrentCriterion) {
				nCriteriaRead++;
				pcPositiveCriterion = PC_SEVERITY;
			}
		}
		
		surveyDetectionProbability = 1.0;
		sprintf(szKey, "%s%s", InterName, "DetectionProbability");
		if (readValueToProperty(szKey, &surveyDetectionProbability, -2, "[0,1]")) {
			if (surveyDetectionProbability < 0.0 || surveyDetectionProbability > 1.0) {
				reportReadError("ERROR: Spatial Statistics DetectionProbability must be in range [0.0, 1.0], read %f\n", surveyDetectionProbability);
			}
		}

		//Set up intervention:
		if(!world->bGiveKeys) {

			if (nCriteriaRead > 1) {
				reportReadError("ERROR: Read multiple PositiveCriteria in %s. Specify at most one.\n", InterName);
			}

			if (bUsingTimesAndFiles && bUsingSingleFile) {
				reportReadError("ERROR: SpatialStatistics set to use both multiple files and times and a single file at multiple times. Use only one method");
				return;
			}

			if (bUsingSingleFile && !bSpecifiedTimeFirst) {
				reportReadError("ERROR: SpatialStatistics set to use sequence of times, but TimeFirst was not set.");
				return;
			}

			if (bUsingTimesAndFiles) {
				for (auto itr = root.begin(); itr != root.end(); ++itr) {

					std::stringstream ssKey;
					ssKey << itr.key();
					std::string sKey = ssKey.str();
					std::replace(sKey.begin(), sKey.end(), '\"', ' ');

					std::stringstream ssClean;
					ssClean << sKey;

					double time;// = itr.key().asDouble();

					if (!(ssClean >> time)) {
						reportReadError("ERROR: Time: %s is not a valid simulation time\n", ssClean.str().c_str());
						return;
					}

					//Either just a string (targetDataFileName), or an object containing a pair of files, one for a targetDataFileName and one for surveyEffortFileName
					std::string targetDataFileName;
					bool bSurveyEffortSpecified = false;
					std::string surveyEffortFileName;

					auto jsonVal = *itr;

					if (jsonVal.isString()) {
						targetDataFileName = jsonVal.asString();
					} else {
						if (!jsonVal.isObject()) {
							reportReadError("ERROR: Elements of SpatialStatisticsTimesAndFiles must be either a single fileName or an object containing {\"TargetData\" : <fileName1>, \"SurveyEffort\" : <fileName2>}\n");
							return;
						}

						for (auto thingItr = jsonVal.begin(); thingItr != jsonVal.end(); ++thingItr) {

							bool bFoundValidKey = false;

							if (thingItr.key().asString() == "TargetData") {

								targetDataFileName = (*thingItr).asString();

								bFoundValidKey = true;
							}

							if (thingItr.key().asString() == "SurveyEffort") {

								surveyEffortFileName = (*thingItr).asString();

								bSurveyEffortSpecified = true;


								bFoundValidKey = true;
							}

							if (!bFoundValidKey) {
								reportReadError("ERROR: Elements of SpatialStatisticsTimesAndFiles must be either a single fileName or an object containing {\"TargetData\" : <fileName1>, \"SurveyEffort\" : <fileName2>}, found: %s.", jsonVal.toStyledString().c_str());
								return;
							}

						}

					}


					auto dataRaster = new Raster<int>();

					if (!dataRaster->init(targetDataFileName)) {
						reportReadError("ERROR: Unable to read SpatialStatistic target data raster file %s.\n", targetDataFileName.c_str());
						return;
					}

					if (!dataRaster->header.sameAs(*world->header)) {
						reportReadError("ERROR: SpatialStatistic target data raster file %s header does not match simulation.\n", targetDataFileName.c_str());
						return;
					}

					if (targetData.find(time) != targetData.end()) {
						reportReadError("ERROR: SpatialStatistics cannot use the same time (%.6f) twice.\n", time);
						return;
					}

					for (const auto &myPair : targetData) {

						double dDataTime = myPair.first;

						if (dDataTime - time == 0.0) {
							reportReadError("ERROR: Time: %.6f is present more than once in the set of SpatialStatistics times. Repetitions of the exact same time are not allowed (even if string representations are distinct).\n", dDataTime);
							return;
						}

					}

					//Finally, actually able to use the data:
					targetData[time] = new TargetData(targetDataFileName, *dataRaster);


					if (bSurveyEffortSpecified) {

						Raster<int> effortRaster;

						if (!effortRaster.init(surveyEffortFileName)) {
							reportReadError("ERROR: Unable to read SpatialStatistic survey effort raster file %s.\n", surveyEffortFileName.c_str());
							return;
						}

						if (!effortRaster.header.sameAs(*world->header)) {
							reportReadError("ERROR: SpatialStatistic survey effort raster file %s header does not match simulation.\n", surveyEffortFileName.c_str());
							return;
						}

						//Finally, actually able to use the data:
						surveyEffortMaps[time] = effortRaster;
					}

				}

				if (targetData.size() == 0) {
					reportReadError("ERROR: No target data found in specified SpatialStatisticsTimesAndFiles: \"%s\". Make sure the json specifies an object that contains pairs of (quoted) time : filename pairs.\n", szConfigStuff);
					return;
				}

				//Frequency is not well defined if using explicit times, but does get in the way when checking to avoid double outputs at the end...
				frequency = 1.0e-6;

			}

			if (bUsingSingleFile) {
				//Read target data file
				auto targetDataRaster = new Raster<int>();
				if (!targetDataRaster->init(std::string(targetFileName))) {
					enabled = 0;
					reportReadError("ERROR: Unable to read file %s for spatial statistics\n", targetFileName);
				}

				//Check that target data matches extent of the world
				if (enabled && !targetDataRaster->header.sameAs(*(world->header))) {
					reportReadError("ERROR: Target Data header does not match landscape header\n");
				}

				singleTargetData = new TargetData(std::string(targetFileName), *targetDataRaster);
			}

			//Allocate the simulation data raster:
			simDataRaster = new Raster<int>();
			simDataRaster->init(*(world->header));

		}

		setupStats();

		timeNext = getNextTime(world->timeStart - 1.0);

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterSpatialStatistics::~InterSpatialStatistics() {
	
	if(singleTargetData != NULL) {
		delete singleTargetData;
		singleTargetData = NULL;
	}

	for (const auto &myPair : targetData) {
		delete myPair.second;
	}

	if(simDataRaster != NULL) {
		delete simDataRaster;
		simDataRaster = NULL;
	}
	
	if(statistics.size() != 0) {
		for(int iStat=0; iStat<statistics.size(); iStat++) {
			if(statistics[iStat] != NULL) {
				delete statistics[iStat];
				statistics[iStat] = NULL;
			}
		}
	}

}

int InterSpatialStatistics::reset() {

	if(enabled) {
		timeNext = getNextTime(world->timeStart - 1.0);

		for(int iStat=0; iStat<statistics.size(); iStat++) {
			statistics[iStat]->reset();
		}
	}

	return enabled;

}

int InterSpatialStatistics::intervene() {
	//SpatialStatistics data

	if(enabled) {

		//Avoid double final line output by preventing writing within 1% output fq before end of simulation
		//Misery here that frequency is not correct when using TimesAndFiles
		if(world->timeEnd - timeNext > 0.01*frequency) {

			//Calculate stats:
			getStats(timeNext);

		}

		timeNext = getNextTime(timeNext);

	}

	return 1;
}

int InterSpatialStatistics::finalAction() {
	//output the final Spatial Statistics data
	//printf("SpatialStatistics::finalAction()\n");

	if(enabled) {

		if (targetData.size() > 0) {
			if (targetData.find(world->timeEnd) == targetData.end()) {
				//We have data for an explicit set of times, and we don't explicitly have data for the end of the simulation, so we won't do anything here
				return 1;
			}
		}

		getStats(world->timeEnd);
	}

	return 1;
}

void InterSpatialStatistics::writeSummaryData(FILE *pFile) {
	return;
}

std::map<std::string, double> InterSpatialStatistics::calculateStats(double tNow, TargetData data, InterSpatialStatistics::SpatialStatisticTypes statistic) {

	if (statistics.size() == 0) {
		printk("ERROR: No spatial statistics are enabled.\n");
		return {};
	}

	if (!statistics[statistic]->enabled) {
		printk("ERROR: Requested a Spatial Statistic that is not enabled: ");
		if (statistic == SS_COUNTS) {
			printk("COUNTS");
		}
		if (statistic == SS_RING) {
			printk("RING");
		}
		if (statistic == SS_MORANI) {
			printk("MORANI");
		}
		if (statistic == SS_CLUSTER) {
			printk("CLUSTER");
		}
		printk("\n");
		return {};
	}

	//Get current state:
	for (int iX = 0; iX<world->header->nCols; iX++) {
		for (int iY = 0; iY<world->header->nRows; iY++) {
			simDataRaster->setValue(iX, iY, nLocationStatus(iX, iY));
		}
	}

	return statistics[statistic]->calculateStats(tNow, data, simDataRaster);
}

int InterSpatialStatistics::setupStats() {

	statistics.resize(SS_MAX + 1);

	statistics[SS_COUNTS]  = new SpatialStatisticCounts();
	statistics[SS_RING]    = new SpatialStatisticRing();
	statistics[SS_MORANI]  = new SpatialStatisticMoranI();
	statistics[SS_CLUSTER] = new SpatialStatisticCluster();

	std::vector<TargetData*> targetDataVector;

	for (const auto &it : targetData) {
		targetDataVector.push_back(it.second);
	}

	for(int iStat=0; iStat<statistics.size(); iStat++) {
		statistics[iStat]->setup(targetDataVector);
	}

	return 1;

}

int InterSpatialStatistics::getStats(double dTimeNow) {

	TargetData *target = singleTargetData;
	if (targetData.size() > 0) {
		if (targetData.find(dTimeNow) == targetData.end()) {
			//We've asked for a time that doesn't exist
			return 0;
		}
		target = targetData[dTimeNow];
	}



	//Get current state:
	if (surveyEffortMaps.find(dTimeNow) != surveyEffortMaps.end()) {
		//We have an explicitly specified survey scheme to follow:

		auto surveyFindings = InterManagement::performSurvey(surveyEffortMaps[dTimeNow], surveyDetectionProbability);

		for (int iX = 0; iX < world->header->nCols; iX++) {
			for (int iY = 0; iY < world->header->nRows; iY++) {

				if (surveyFindings(iX, iY) == surveyFindings.header.NODATA) {
					//Not surveyed:
					simDataRaster->setValue(iX, iY, -1);
				} else {
					if (surveyFindings(iX, iY) > 0) {
						//If one or more finding: positive
						simDataRaster->setValue(iX, iY, 1);
					} else {
						//No findings: negative:
						simDataRaster->setValue(iX, iY, 0);
					}
				}
				
			}
		}

	} else {
		//Use the actual simulation state
		for (int iX = 0; iX<world->header->nCols; iX++) {
			for (int iY = 0; iY<world->header->nRows; iY++) {

				int locStatus = nLocationStatus(iX, iY);

				//Simple survey
				if(locStatus == 1) {
					//Actually positive, do simple survey:
					if (dUniformRandom() < surveyDetectionProbability) {
						//Correctly classified:
						simDataRaster->setValue(iX, iY, 1);
					} else {
						//Missed it:
						simDataRaster->setValue(iX, iY, 0);
					}
				} else {
					//No false positives, so do if not actually positive, just use the value directly
					simDataRaster->setValue(iX, iY, locStatus);
				}

				
			}
		}
	}

	
	for (int iStat = 0; iStat < statistics.size(); iStat++) {
		statistics[iStat]->getStats(dTimeNow, *target, simDataRaster);
	}

	return 1;
}


int InterSpatialStatistics::nLocationStatus(int xPos, int yPos) {

	int targetValue = -1;
	if(world->locationArray[xPos + yPos*world->header->nCols] != NULL) {
		targetValue = 0;
		switch(pcPositiveCriterion) {
		case PC_SYMPTOMATIC:
			if(world->locationArray[xPos + yPos*world->header->nCols]->getDetectableCoverageProportion(-1) > 0.0) {
				targetValue = 1;
			}
			break;
		case PC_SEVERITY:
			if(world->locationArray[xPos + yPos*world->header->nCols]->getEpidemiologicalMetric(-1, PopulationCharacteristics::severity) > 0.0) {
				targetValue = 1;
			}
			break;
		default:
			printf("ERROR: Unrecognised positive criterion in %s.\n", InterName);
			targetValue = -1;
			break;
		}
	}

	return targetValue;
}

double InterSpatialStatistics::getNextTime(double dTimeNow) {

	//This is only called to reset, at which point dTimeNow = -1.0, or upon every use, in which case dTimeNow is one of the sequence of calling times

	//In the fixed time sequence version:
	if (targetData.size() == 0) {
		if (dTimeNow < world->timeStart) {
			return timeFirst;
		}

		return dTimeNow + frequency;
	}

	//In the explicit times version:
	//Find the smallest distance into the future time:
	double dMinFutureDistance = 1e30;
	double dNextTime = world->timeStart - 1.0;

	for (const auto &myPair : targetData) {

		double dDataTime = myPair.first;

		double dFutureDistance = dDataTime - dTimeNow;

		if (dFutureDistance > 0.0 && dFutureDistance < dMinFutureDistance) {
			dMinFutureDistance = dFutureDistance;
			dNextTime = dDataTime;
		}
	}

	if (dNextTime != world->timeStart - 1.0) {
		return dNextTime;
	}

	//If there weren't any future times, we don't do anything
	return world->timeEnd + 1e30;
}
