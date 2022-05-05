//InterDetectionSweep.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "ListManager.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "RateTree.h"
#include "RasterHeader.h"
#include "myRand.h"
#include "InterDetectionSweep.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterDetectionSweep::InterDetectionSweep() {
	timeSpent = 0;
	InterName = "DetectionSweep";
	totalCost = 0.0;

	setCurrentWorkingSubection(InterName, CM_POLICY);

	//Read Data:

	enabled = 0;
	readValueToProperty("DetectionEnable",&enabled,-1, "[0,1]");

	if(enabled) {

		frequency = 1.0;
		readValueToProperty("DetectionFrequency",&frequency,2, ">0");

		timeFirst = world->timeStart + frequency;
		readValueToProperty("DetectionFirst",&timeFirst,2, ">=0");
		timeNext = timeFirst;

		if (timeFirst < world->timeStart) {
			printk("Warning: DetectionFirst (%f) was specified for before the simulation start time (%f). No detection will be applied before the start of the simulation.\n", timeFirst, world->timeStart);

			//Advance to first time greater than or equal to simulation start:
			int nPeriodsBehind = ceil((world->timeStart - timeFirst) / frequency);
			double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

			printk("Warning: Advancing DetectionFirst time to earliest time >= simulation start time by advancing start time %d periods to %f.\n", nPeriodsBehind, newTimeFirst);

			timeFirst = newTimeFirst;
		}

		prob = 1.0;
		readValueToProperty("DetectionParameter",&prob,2, "[0,1]");
		if (prob < 0.0 || prob > 1.0) {
			reportReadError("ERROR: DetectionParameter must be in the range [0,1], read: %.6f\n", prob);
		}

		bDetectionProbabilityRaster = 0;
		readValueToProperty("DetectionUseDetectionParameterRaster", &bDetectionProbabilityRaster, -2, "[0,1]");

		char szDetectionParameterRasterName[_N_MAX_STATIC_BUFF_LEN];
		sprintf(szDetectionParameterRasterName, "DetectionParameterRaster.txt");
		if(bDetectionProbabilityRaster) {
			readValueToProperty("DetectionParameterRasterFileName", szDetectionParameterRasterName, -3, "#FileName#");
		}

		minDetectableProportion = 0.0;
		readValueToProperty("DetectionMinumumDetectableProportionThreshold", &minDetectableProportion, -2, "[0,1]");

		cost = 1.0;
		readValueToProperty("DetectionCost",&cost,-2, ">=0");

		dynamic = 0;
		if(readValueToProperty("DetectionDynamic",&dynamic,-2, "[0,1]")) {

			radius = 1.0;
			readValueToProperty("DetectionSearchRadius",&radius,3, ">0");

		}

		trackInfectionLevels = 0;
		readValueToProperty("DetectionTracking",&trackInfectionLevels,-2, "[0,1]");

		samplingStrategy = SAMPLING_SEARCHLIST;
		int tmpInt = 0;
		if(readValueToProperty("DetectionUseWeighting",&tmpInt,-2, "[0,1]")) {
			if(tmpInt) {
				samplingStrategy = SAMPLING_FROMWEIGHTING;
			}
		}

		if(samplingStrategy == SAMPLING_FROMWEIGHTING || world->bGiveKeys) {

			nSurveyWeightings = 1;
			readValueToProperty("DetectionSurveyNWeightings", &nSurveyWeightings, -3, ">0");
			if (nSurveyWeightings <= 0) {
				reportReadError("Error: Cannot less than one survey: DetectionSurveyNWeightings %d\n", nSurveyWeightings);
			}

			nSurveySize = 1;
			readValueToProperty("DetectionSurveySize",&nSurveySize,3, ">0");
			if(nSurveySize <= 0) {
				reportReadError("Error: Cannot perform survey of less than one point: DetectionSurveySize %d\n",nSurveySize);
			}

		}

		surveyStyle = SURVEY_OLD;
		tmpInt = 0;
		if(readValueToProperty("DetectionUseNewSurvey",&tmpInt,-2, "[0,1]")) {
			if(tmpInt) {
				surveyStyle = SURVEY_NEW;
			}
		}

		if(surveyStyle == SURVEY_NEW || world->bGiveKeys) {
			nMaxPopSize = 1;
			readValueToProperty("DetectionMaxPopSize",&nMaxPopSize,3, ">0");
			if(nMaxPopSize <= 0) {
				reportReadError("ERROR: Population size not positive: DetectionMaxPopSize %d\n",nMaxPopSize);
			}

			int bReadSize = 0;
			nSampleSize = 1;
			bReadSize = readValueToProperty("DetectionSampleSize",&nSampleSize,-3, ">0");
			if(bReadSize && nSampleSize <= 0) {
				reportReadError("ERROR: Survey sample size not positive: DetectionSampleSize %d\n",nSampleSize);
			}
			if(bReadSize && nSampleSize > nMaxPopSize) {
				printk("Warning: Survey sample size larger than population size: DetectionSampleSize %d, DetectionMaxPopSize %d\n", nSampleSize, nMaxPopSize);
			}

			int bReadDensity = 0;
			fSampleProportion = 1.0;
			bReadDensity = readValueToProperty("DetectionSampleProportion",&fSampleProportion,-3, "[0,1]");

			if(bReadDensity) {
				if(fSampleProportion < 0.0 || fSampleProportion > 1.0) {
					reportReadError("ERROR: Sample Proportion must be in the interval [0.0, 1.0]: DetectionSampleProportion %f\n",fSampleProportion);
				}
			}

			if(bReadDensity == bReadSize && !bIsKeyMode()) {
				reportReadError("ERROR: Must read exactly one of DetectionSampleSize or DetectionSampleProportion. Read ");
				if(bReadDensity) {
					reportReadError("both.\n");
				} else {
					reportReadError("neither.\n");
				}
			}

		}

		if (bDetectionProbabilityRaster && !world->bGiveKeys) {
			if (!probRaster.init(szDetectionParameterRasterName)) {
				reportReadError("ERROR: DetectionParameterRaster unable to initialise from raster file \"%s\"\n", szDetectionParameterRasterName);

				if (!probRaster.header.sameAs(*world->header)) {
					reportReadError("ERROR: DetectionParameterRaster header does not match landscape header\n");
				}

				for (int iRow = 0; iRow < probRaster.header.nRows; ++iRow) {
					for (int iCol = 0; iCol < probRaster.header.nCols; ++iCol) {
						double probVal = probRaster(iCol, iRow);
						if (probVal != probRaster.header.NODATA && (probVal < 0.0 || probVal > 1.0)) {
							reportReadError("ERROR: DetectionParameterRaster \"%s\" had an invalid value (%.6f) outside range [0,1] at row=%d, col=%d.\n", szDetectionParameterRasterName, probVal, iRow, iCol);
						}
					}
				}

			}
		}

		//TODO: Option to survey on an individual population basis?
		iPopulationToSurvey = -1;

		bOuputStatus = 0;
		readValueToProperty("DetectionOutputFinalState", &bOuputStatus, -2, "[0,1]");

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

	//Set up intervention:
	if(enabled && !world->bGiveKeys) {

		setupOutput();

		if(samplingStrategy == SAMPLING_FROMWEIGHTING) {
			//Read searchWeightings
			//Raster file L_SEARCHWEIGHTING.txt
			//Contain weightings raster in temporary array of all data:
			int nElements = world->header->nCols*world->header->nRows;
			surveyWeightings.resize(nSurveyWeightings);

			for (int iWeighting = 0; iWeighting < nSurveyWeightings; ++iWeighting) {

				char tempWeightFileName[_N_MAX_STATIC_BUFF_LEN];
				sprintf(tempWeightFileName, "L_SEARCHWEIGHTING_%d.txt", iWeighting);
				vector<double> tempWeightings(nElements);

				//Read raster:
				if (!readRasterToArray(tempWeightFileName, &tempWeightings[0])) {
					reportReadError("Error reading \"%s\" raster for InterDetectionSweep\n", tempWeightFileName);
				} else {

					//Find how many are interesting:
					surveyWeightings[iWeighting].nWeighted = 0;
					for (int i = 0; i<nElements; i++) {
						if (tempWeightings[i] > 0.0) {
							//Check if location is actually populated:
							if (world->locationArray[i] != NULL) {
								surveyWeightings[iWeighting].nWeighted++;
							}
						}
					}

					if (surveyWeightings[iWeighting].nWeighted > 0) {

						//Allocate permanent array:
						surveyWeightings[iWeighting].weightings.resize(surveyWeightings[iWeighting].nWeighted);
						surveyWeightings[iWeighting].locationIndex.resize(surveyWeightings[iWeighting].nWeighted);
						int nActive = 0;
						for (int i = 0; i<nElements; i++) {
							if (tempWeightings[i] > 0.0) {
								//Check if location is actually populated:
								if (world->locationArray[i] != NULL) {
									surveyWeightings[iWeighting].locationIndex[nActive] = i;
									surveyWeightings[iWeighting].weightings[nActive] = tempWeightings[i];
									nActive++;
								}
							}
						}

					} else {
						//Weren't actually any places to survey:
						reportReadError("Error: No locations in weighted search raster \"%s\" have positive weights or are all unpopulated, unable to survey.\n", tempWeightFileName);
					}

				}
			}

			//DEBUG:
			//writeNewFile("DEBUG_WEIGHT_SAMPLE.txt");

		}

	}
}

InterDetectionSweep::~InterDetectionSweep() {
	
}

int InterDetectionSweep::setupOutput() {

	iSurveyRound = 0;

	char tempDetectionFileName[N_MAXFNAMELEN];
	sprintf(tempDetectionFileName, "%sDETECTIONData.txt", szPrefixOutput());

	detectionFileName = string(tempDetectionFileName);

	if(trackInfectionLevels) {
		//Create output file for infection levels
		writeDetectionHeaders();
	}

	return 1;

}

int InterDetectionSweep::writeSurveyReport(const SurveyReport &report, int iSurveyRound, int iSurveyWeighting) {

	//TODO: Write this data all to one file. Too many files created to be practical in batch runs
	char tempFileName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(tempFileName, "%sSURVEY_WEIGHTING_%d%s", szPrefixOutput(), iSurveyWeighting, world->szSuffixTextFile);

	if (iSurveyRound == 0) {
		writeNewFile(tempFileName);

		printToFileAndScreen(tempFileName, 0, "Time\tnLocationsSurveyed\tnInfectedLocationsFound\n");
	}
	printToFileAndScreen(tempFileName, 0, "%12.12f\t%12d\t%12d\n", timeNext, report.nLocationsSurveyed, report.nInfectedLocationsFound);

	return 0;
}

int InterDetectionSweep::intervene() {

	//printf("SEARCHING LANDSCAPE!\n");

	//Detection Sweep

	if(trackInfectionLevels) {
		writeDetectionData(0);
	}

	switch(samplingStrategy) {
	case SAMPLING_SEARCHLIST:
		//Nothing to do
		break;
	case SAMPLING_FROMWEIGHTING:

		for (int iWeighting = nSurveyWeightings - 1; iWeighting >= 0; --iWeighting) {// Do it in this order so that we always get the zeroth survey as the one actually used

			//Clear existing search allocation:
			clearLandscapeSearchStatus();

			//Calculate new search allocation
			performWeightedSample(surveyWeightings[iWeighting]);

			if (iWeighting != 0) {
				auto testSurveyReport = performSurvey(false);
				if (trackInfectionLevels) {
					writeSurveyReport(testSurveyReport, iSurveyRound, iWeighting);
				}
			}

		}

		break;
	default:
		reportReadError("Error: Unknown sampling strategy: %d",samplingStrategy);
		break;
	}

	//Actually perform the survey:
	auto realSurveyReport = performSurvey();

	if(trackInfectionLevels) {
		writeDetectionData(1);
		writeSurveyReport(realSurveyReport, iSurveyRound, 0);
	}

	iSurveyRound++;

	timeNext += frequency;

	return 1;
}

int InterDetectionSweep::reset() {

	if(enabled) {

		setupOutput();

		//Reset all locations to default status
		clearLandscapeKnownStatus();

		//TODO: handle if isKnown and isSearched are present and for detectionDynamic:
		if(dynamic) {
			//Clear isSearched values:
			clearLandscapeSearchStatus();

			//Reread initial detection search status:
			//...

			printC(1,"Dynamic detection is not yet compatible with risk maps. Please complain to Rich.\n");
		}

		timeNext = timeFirst;

	}

	return enabled;

}

InterDetectionSweep::SurveyReport InterDetectionSweep::performSurvey(bool actualSurvey) {

	ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
	LocationMultiHost *activeLocation;

	SurveyReport surveyReport = {};

	switch(surveyStyle) {
	case SURVEY_OLD:
		//Pass over all active locations
		while(pNode != NULL) {
			activeLocation = pNode->data;
			//if is currently meant to be searched
			if(activeLocation->getSearch(iPopulationToSurvey)) {
				//If Location has detectables and not previously known infected
				if(!activeLocation->getKnown(iPopulationToSurvey)) {
					if (actualSurvey) {
						totalCost += cost;
					}
					surveyReport.nLocationsSurveyed++;
					//attempt to detect infected
					double u = activeLocation->getDetectableCoverageProportion(iPopulationToSurvey);
					if(u > minDetectableProportion) {

						double detectionParameter = getDetectionParameterForLocation(activeLocation);

						double v = pow(detectionParameter, 2 - u);
						if(dUniformRandom() < v) {
							if (actualSurvey) {
								activeLocation->setKnown(true, iPopulationToSurvey);
							}
							surveyReport.nInfectedLocationsFound++;
						}
					}
				}
			}
			pNode = pNode->pNext;
		}
		break;
	case SURVEY_NEW:
		//Pass over all active locations
		while(pNode != NULL) {
			activeLocation = pNode->data;
			//if is currently meant to be searched
			if(activeLocation->getSearch(iPopulationToSurvey)) {
				//if has detectables and not previously known infected
				if(!activeLocation->getKnown(iPopulationToSurvey)) {
					if (actualSurvey) {
						totalCost += cost;
					}
					surveyReport.nLocationsSurveyed++;
					//attempt to detect infected
					double detectableCoverage = activeLocation->getDetectableCoverageProportion(iPopulationToSurvey);
					if(detectableCoverage > 0) { // else automatically fails
						int nHosts = int(ceil(nMaxPopSize*activeLocation->getCoverage(iPopulationToSurvey)));
						int nDetectable = int(ceil(nHosts*detectableCoverage));
						//Density dependent sampling:
						int nActualSamples = nSampleSize;
						if(fSampleProportion > 0.0) {
							nActualSamples = int(ceil(fSampleProportion*nHosts));
						}

						int bInfectionFound = 0;
						//For each host to be sampled:
						for(int i=0; i<nActualSamples; i++) {
							//See if host is actually infected
							//Probability nDetectable/nHosts
							if(dUniformRandom()*nHosts < (double)nDetectable) {
								//Have removed a detectable from the pool:
								nDetectable--;
								//Was infected, see if it is successfully identified:
								double detectionParameter = getDetectionParameterForLocation(activeLocation);
								if(dUniformRandom() < detectionParameter) {
									bInfectionFound = 1;
									//Only looking for presence absence, so don't need to bother with further trials:
									break;
								}
							}
							//Removed a host from the pool:
							nHosts--;
						}


						if(bInfectionFound) {
							if (actualSurvey) {
								activeLocation->setKnown(true, iPopulationToSurvey);
							}
							surveyReport.nInfectedLocationsFound++;
						}
					}
				}
			}
			pNode = pNode->pNext;
		}
		break;
	default:
		reportReadError("ERROR: Unknown survey style: %d\n");
		break;
	}

	return surveyReport;
}


int InterDetectionSweep::clearLandscapeSearchStatus() {

	ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
	LocationMultiHost *activeLocation;

	//Pass over all active locations
	while(pNode != NULL) {
		activeLocation = pNode->data;
		activeLocation->setSearch(false,-1);
		pNode = pNode->pNext;
	}

	return 0;
}


int InterDetectionSweep::clearLandscapeKnownStatus() {

	ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
	LocationMultiHost *activeLocation;

	//Pass over all active locations
	while(pNode != NULL) {
		activeLocation = pNode->data;
		activeLocation->setKnown(false,-1);
		pNode = pNode->pNext;
	}

	return 0;
}

int InterDetectionSweep::performWeightedSample(const SurveyWeightingScheme &weightingScheme) {

	auto sampleLocations = sample_BT(weightingScheme);

	return allocateSearchStatus(sampleLocations);

}

vector<int> InterDetectionSweep::sample_BT(const SurveyWeightingScheme &weightingScheme) {

	//Work out how many samples are actually going to be taken:
	int nSampleLocations = min(nSurveySize, weightingScheme.nWeighted);
	vector<int> pSampleLocations(nSampleLocations);

	RateTree *weightTree = new RateTree(weightingScheme.nWeighted);

	//Submit all the weightings to the structure:
	for (int i = 0; i<weightingScheme.nWeighted; i++) {
		weightTree->submitRate_dumb(i, weightingScheme.weightings[i]);
	}

	weightTree->fullResum();

	int nPick = 0;
	while(nPick < nSampleLocations) {

		//Select a location
		double dTotalWeight = weightTree->getTotalRate();

		double dRandomWeight = dUniformRandom()*dTotalWeight;

		int nIndex = weightTree->getLocationEventIndex(dRandomWeight);

		//Rounding error check:
		if(weightTree->getRate(nIndex) > 0.0) {
			//Valid, use it and move on to the next
			pSampleLocations[nPick] = weightingScheme.locationIndex[nIndex];

			nPick++;

			//Don't want to pick the same location twice:
			weightTree->submitRate(nIndex, 0.0);
		}
	}

	delete weightTree;

	//DEBUG:
	//Output index of location searched to file:
	/*for(int i=0; i<nSampleLocations; i++) {
	printToFileAndScreen("DEBUG_WEIGHT_SAMPLE.txt",0,"%d ",pSampleLocations[i]);
	}
	printToFileAndScreen("DEBUG_WEIGHT_SAMPLE.txt",0,"\n");*/

	return pSampleLocations;
}

int InterDetectionSweep::allocateSearchStatus(const vector<int> &sampleLocationIndices) {

	for (const int locationIndex : sampleLocationIndices) {
		world->locationArray[locationIndex]->setSearch(true, iPopulationToSurvey);
	}

	return 0;
}

double InterDetectionSweep::getDetectionParameterForLocation(LocationMultiHost *loc) {

	double actualParameter = prob;
	if (bDetectionProbabilityRaster) {
		actualParameter = probRaster(loc->xPos, loc->yPos);
		if (actualParameter == probRaster.header.NODATA) {
			actualParameter = prob;//Default to the global parameter where NODATA
		}
	}

	return actualParameter;
}

int InterDetectionSweep::writeDetectionHeaders() {

	writeNewFile(detectionFileName.c_str());

	printToFileAndScreen(detectionFileName.c_str(), 0, "Time\tKnownLocations\tGlobalKnownInfectious\tGlobalKnownDetectable\n");

	return 0;
}

int InterDetectionSweep::writeDetectionData(int afterSurvey) {

	nGlobalKnownInfectious = 0.0;
	nGlobalKnownDetectable = 0.0;
	nLKnown = 0;


	ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
	LocationMultiHost *activeLocation;

	//Pass over all active locations
	while(pNode != NULL) {
		activeLocation = pNode->data;
		//See if any infection found:
		if(activeLocation->getKnown(iPopulationToSurvey)) {
			nLKnown++;
			//Total quantity (Land area) of infectious material (from all populations) in those locations:
			nGlobalKnownInfectious += activeLocation->getInfectiousCoverageProportion(-1);
			nGlobalKnownDetectable += activeLocation->getDetectableCoverageProportion(-1);
		}
		pNode = pNode->pNext;
	}

	//This fudges the before/after survey time by adding a small time: 10^-5*surveyFrequency
	printToFileAndScreen(detectionFileName.c_str(),0,"%12.12f\t%12d\t%12.6f\t%12.6f\n",timeNext+0.00001*frequency*afterSurvey,nLKnown,nGlobalKnownInfectious,nGlobalKnownDetectable);

	return 0;
}

int InterDetectionSweep::finalAction() {
	//output the final Detection sweep data
	//printf("Creating Detection Rasters\n");

	if(enabled) {

		char fName[256];

		//Output rasters of locations detected+to be searched
		if(enabled && bOuputStatus) {
			for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
				sprintf(fName,"%s%s%d_KNOWN%s", szPrefixOutput(), world->szPrefixLandscapeRaster, i, world->szSuffixTextFile);
				writeRasterPopulationProperty(fName, i, &LocationMultiHost::getKnown);

				sprintf(fName,"%s%s%d_SEARCH%s", szPrefixOutput(), world->szPrefixLandscapeRaster, i, world->szSuffixTextFile);
				writeRasterPopulationProperty(fName, i, &LocationMultiHost::getSearch);
			}
		}

	}

	return 1;
}

void InterDetectionSweep::writeSummaryData(FILE *pFile) {
	
	fprintf(pFile,"%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Cost %12.12f\n\n",totalCost);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
