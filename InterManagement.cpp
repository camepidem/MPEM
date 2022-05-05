
#include "stdafx.h"
#include "json/json.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "myRand.h"
#include "InterManagement.h"



InterManagement::InterManagement() {

	timeSpent = 0;
	InterName = "Management";

	//Memory housekeeping:
	

	setCurrentWorkingSubection(InterName, CM_POLICY);

	char szKey[N_MAXSTRLEN];

	enabled = 0;

	sprintf(szKey, "%s%s", InterName, "Enable");

	readValueToProperty(szKey, &enabled, -1, "[0,1]");

	if (enabled) {

		sprintf(szKey, "%s%s", InterName, "DetectionProbability");
		pDetection = 1.0;
		if (readValueToProperty(szKey, &pDetection, -2, "[0.0, 1.0]")) {
			if (pDetection < 0.0 || pDetection > 1.0) {
				reportReadError("ERROR: %s (%.6f) is outside of valid range for a probability [0.0, 1.0].\n", szKey, pDetection);
			}
		}

		sprintf(szKey, "%s%s", InterName, "LogSurveyDataTable");
		bLogSurveyDataTable = 1;
		if (readValueToProperty(szKey, &bLogSurveyDataTable, -2, "[0, 1]")) {
			if (bLogSurveyDataTable < 0 || bLogSurveyDataTable > 1) {
				reportReadError("ERROR: %s (%.6f) is outside of valid range [0, 1].\n", szKey, bLogSurveyDataTable);
			}
		}

		//Multiple target data files:
		Json::Reader reader;
		Json::Value root;

		char szConfigStuff[_N_MAX_DYNAMIC_BUFF_LEN];
		sprintf(szConfigStuff, "{\"1.0\" : \"myTargetSurveillanceRasterName.txt\"}");
		sprintf(szKey, "%s%s", InterName, "SurveillanceTimesAndFiles");
		int bUsingTimesAndFiles = readValueToProperty(szKey, szConfigStuff, 2, "~#JSON{f:#FileName#}#~");
		if (bUsingTimesAndFiles && !bIsKeyMode()) {
			if (!reader.parse(szConfigStuff, root)) {
				reportReadError("ERROR: Unable to parse json string (you may need to put double quotes around the times, also note that single quotes are not valid JSON): %s, message: %s\n", szConfigStuff, reader.getFormattedErrorMessages().c_str());
				return;
			}

			if (!root.isObject()) {

				if (root.isString()) {
					//TODO: Could do a last chance parse here but could pathologically need to recursively parse forever
					reportReadError("ERROR: ManagementSurveillanceTimesAndFiles was a JSON string (%s), not a JSON object (perhaps don't enclose the whole thing in double quotes?)\n", szConfigStuff);
					return;
				}

				reportReadError("ERROR: ManagementSurveillanceTimesAndFiles Unable to parse JSON to JSON object: %s\n", szConfigStuff);
				return;
			}

		}

		if (!bIsKeyMode()) {

			for (auto itr = root.begin(); itr != root.end(); ++itr) {

				//std::cout << itr.key() << " : " << *itr << std::endl;

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

				std::string fileName = (*itr).asString();

				auto dataRaster = new Raster<int>();

				if (!dataRaster->init(fileName)) {
					reportReadError("ERROR: Management Unable to read target data raster file %s.\n", fileName.c_str());
					return;
				}

				if (!dataRaster->header.sameAs(*world->header)) {
					reportReadError("ERROR: Management target data raster file %s header does not match simulation.\n", fileName.c_str());
					return;
				}

				bool bDataError = false;
				for (auto iRow = 0; iRow < dataRaster->header.nRows; ++iRow) {
					for (auto iCol = 0; iCol < dataRaster->header.nCols; ++iCol) {
						auto dataVal = dataRaster->operator()(iCol, iRow);
						if (dataVal != dataRaster->header.NODATA) {
							if (dataVal < 0) {
								reportReadError("ERROR: Management target data raster file %s value at x=%d y=%d (%d) is negative.\n", fileName.c_str(), iCol, iRow, dataVal);
								bDataError = true;
							}
							if (dataVal > 0) {
								if (world->locationArray[iCol + iRow * dataRaster->header.nCols] == NULL) {
									reportReadError("ERROR: Management target data raster file %s value at x=%d y=%d (%d) is non zero, but there is no host at this location.\n", fileName.c_str(), iCol, iRow, dataVal);
									bDataError = true;
								} else {
									int nHosts = ceil(world->locationArray[iCol + iRow * dataRaster->header.nCols]->getCoverage(-1) * PopulationCharacteristics::nMaxHosts);
									if (dataVal > nHosts) {
										reportReadError("ERROR: Management target data raster file %s value at x=%d y=%d (%d) is greater than the number of host units at this location (%d).\n", fileName.c_str(), iCol, iRow, dataVal, nHosts);
										bDataError = true;
									}
								}
							}
						}
					}
				}

				if (bDataError) {
					reportReadError("ERROR: Management target data raster file %s contains invalid values.\n", fileName.c_str());
					return;
				}

				if (surveySchedule.find(time) != surveySchedule.end()) {
					reportReadError("ERROR: Management cannot use the same time (%.6f) twice.\n", time);
					return;
				}

				for (const auto &myPair : surveySchedule) {

					double dDataTime = myPair.first;

					if (dDataTime - time == 0.0) {
						reportReadError("ERROR: Time: %.6f is present more than once in the set of Management times. Repetitions of the exact same time are not allowed (even if string representations are distinct).\n", dDataTime);
						return;
					}

				}

				//Finally, actually able to use the data:
				surveySchedule[time] = dataRaster;

			}

			if (surveySchedule.size() == 0) {
				reportReadError("ERROR: No target data found in specified SpatialStatisticsTimesAndFiles: \"%s\". Make sure the json specifies an object that contains pairs of (quoted) time : filename pairs.\n", szConfigStuff);
				return;
			}

		}

		timeNext = getNextTime(world->timeStart - 1.0);

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}


InterManagement::~InterManagement() {

	for (const auto &myPair : surveySchedule) {
		delete myPair.second;
	}

}

int InterManagement::intervene() {

	if (enabled) {

		doSurvey(timeNext);

		timeNext = getNextTime(timeNext);

	}

	return 1;
}

Raster<int> InterManagement::performSurvey(const Raster<int> &surveyRaster, double detectionProbability) {

	Raster<int> surveyFindings(surveyRaster.header, true);

	for (auto iRow = 0; iRow < surveyRaster.header.nRows; ++iRow) {
		for (auto iCol = 0; iCol < surveyRaster.header.nCols; ++iCol) {
			auto nHostsSurveyed = surveyRaster(iCol, iRow);
			if (nHostsSurveyed != surveyRaster.header.NODATA && nHostsSurveyed > 0) {
				//Do survey at this location:
				auto targetLoc = world->locationArray[iCol + iRow * surveyRaster.header.nCols];

				int nHosts = ceil(targetLoc->getCoverage(-1) * PopulationCharacteristics::nMaxHosts);
				int nActualSymptomatic = ceil(targetLoc->getDetectableCoverageProportion(-1) * PopulationCharacteristics::nMaxHosts);

				//Sample hosts to look at:
				int nSymptomaticUnpicked = nActualSymptomatic;
				int nHostsToSurvey = nHosts;
				int nSymptomaticInSurvey = 0;
				for (int iHostPicked = 0; iHostPicked < nHostsSurveyed && nSymptomaticUnpicked > 0 && nHostsToSurvey > 0; ++iHostPicked) {
					//Do sampling for hosts without replacement:
					double dProportionSymptomatic = double(nSymptomaticUnpicked) / double(nHostsToSurvey);
					if (dUniformRandom() < dProportionSymptomatic) {
						nSymptomaticInSurvey++;
						nSymptomaticUnpicked--;
					}
					nHostsToSurvey--;
				}

				//Survey hosts:
				int nActualDetections = nSlowBinomial(nSymptomaticInSurvey, detectionProbability);

				int nDetectionFailures = nSymptomaticInSurvey - nActualDetections;

				surveyFindings(iCol, iRow) = nActualDetections;
				
			}
		}
	}

	return surveyFindings;
}

void InterManagement::doSurvey(double dTimeNow) {

	char szSurveyTableFileName[_N_MAX_STATIC_BUFF_LEN];
	if (bLogSurveyDataTable) {
		//Output Table file:
		sprintf(szSurveyTableFileName, "%sManagement_SurveyResults_Time_%.6f.txt", szPrefixOutput(), dTimeNow);

		writeNewFile(szSurveyTableFileName);

		//Header:
		printToFileAndScreen(szSurveyTableFileName, 0, "X\tY\tNHosts\tNHostsSymptomatic\tNHostsSurveyed\tNHostsSurveyDetections\n");
	}

	auto dataRaster = surveySchedule[dTimeNow];

	auto surveyResult = performSurvey(*dataRaster, pDetection);

	for (auto iRow = 0; iRow < dataRaster->header.nRows; ++iRow) {
		for (auto iCol = 0; iCol < dataRaster->header.nCols; ++iCol) {
			auto nHostsSurveyed = dataRaster->operator()(iCol, iRow);
			if (nHostsSurveyed != dataRaster->header.NODATA && nHostsSurveyed > 0) {
				//Get information for this location:
				auto targetLoc = world->locationArray[iCol + iRow * dataRaster->header.nCols];

				int nHosts = ceil(targetLoc->getCoverage(-1) * PopulationCharacteristics::nMaxHosts);
				int nActualSymptomatic = ceil(targetLoc->getDetectableCoverageProportion(-1) * PopulationCharacteristics::nMaxHosts);

				//Survey hosts:
				int nActualDetections = surveyResult(iCol, iRow);
				
				printToFileAndScreen(szSurveyTableFileName, 0, "%d\t%d\t%d\t%d\t%d\t%d\n", iCol, iRow, nHosts, nActualSymptomatic, nHostsSurveyed, nActualDetections);
			}
		}
	}

	return;
}

int InterManagement::reset() {

	if (enabled) {
		timeNext = getNextTime(world->timeStart - 1.0);
	}

	return enabled;
}

int InterManagement::finalAction() {
	
	if (enabled) {

		if (timeNext == world->timeEnd) {
			doSurvey(timeNext);
			timeNext = getNextTime(timeNext);
		}

	}

	return 1;
}

double InterManagement::getNextTime(double dTimeNow) {

	//This is only called to reset, at which point dTimeNow = world->timeStart - 1.0, or upon every use, in which case dTimeNow is one of the sequence of calling times

	//Find the smallest distance into the future time:
	double dMinFutureDistance = world->timeEnd + 1e30;
	double dNextTime = world->timeStart - 1.0;

	for (const auto &myPair : surveySchedule) {

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

void InterManagement::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n", InterName);

	if (enabled) {
		fprintf(pFile, "Enabled\n\n");
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
