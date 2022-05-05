//InterInitialInf.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "RateTree.h"
#include "ListManager.h"
#include "myRand.h"
#include "RasterHeader.h"
#include "InterInitialInf.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterInitialInf::InterInitialInf() {
	timeSpent = 0;
	InterName = "RandomInitialInfection";


	setCurrentWorkingSubection(InterName, CM_SIM);

	enabled = 0;
	readValueToProperty("RandomStartLocationEnable",&enabled,-1, "[0,1]");

	nLocations = 1;
	readValueToProperty("RandomStartLocationNumber",&nLocations,-2, ">0");

	weightingChoice = WEIGHT_HOSTDENSITY;
	int tmpInt = 0;
	if(readValueToProperty("RandomStartLocationWeightHostPresence",&tmpInt,-2, "[0,1]")) {
		if(tmpInt) {
			weightingChoice = WEIGHT_HOSTPRESENCE;
		}
	}

	tmpInt = 1;
	if(readValueToProperty("RandomStartLocationWeightHostDensity",&tmpInt,-2, "[0,1]")) {
		if(tmpInt) {
			weightingChoice = WEIGHT_HOSTDENSITY;
		}
	}

	tmpInt = 0;
	if(readValueToProperty("RandomStartLocationWeightHostDensity_x_Susceptibility",&tmpInt,-2, "[0,1]")) {
		if(tmpInt) {
			weightingChoice = WEIGHT_HOSTDENSITY_x_SUSCEPTIBILITY;
		}
	}

	tmpInt = 0;
	if(readValueToProperty("RandomStartLocationWeightRaster",&tmpInt,-2, "[0,1]")) {
		if(tmpInt) {
			weightingChoice = WEIGHT_BYRASTER;
		}
	}

	char weightRasterName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(weightRasterName, "L_STARTWEIGHTING.txt");

	if (weightingChoice == WEIGHT_BYRASTER || world->bGiveKeys) {
		readValueToProperty("RandomStartLocationWeightRasterFileName", weightRasterName, -3, "#FileName#");
	}


	if(enabled && !world->bGiveKeys) {
		frequency = 1e30;
		timeFirst = world->timeStart;
		timeNext = timeFirst;

		if(weightingChoice != WEIGHT_BYRASTER) {

			int bByPresence = 0;
			int bByDensity = 0;
			int bUseSusceptibility = 0;

			if(weightingChoice == WEIGHT_HOSTPRESENCE) {
				bByPresence = 1;
			}

			if(weightingChoice == WEIGHT_HOSTDENSITY) {
				bByDensity = 1;
			}

			if(weightingChoice == WEIGHT_HOSTDENSITY_x_SUSCEPTIBILITY) {
				bUseSusceptibility = 1;
			}

			//Establish weightings of all locations in landscape:
			nWeighted = 0;
			int nElements = world->header->nCols*world->header->nRows;
			for(int i=0; i<nElements; i++) {
				//Check if location is actually populated:
				LocationMultiHost *loc = world->locationArray[i];
				if(loc != NULL) {
					double testWeight = bByPresence + bByDensity*loc->getCoverage(-1) + bUseSusceptibility*loc->getSusceptibilityWeightedCoverage(-1);
					if(testWeight > 0.0) {
						nWeighted++;
					}
				}
			}

			printk("nWeighted=%d, nActive=%d\n",nWeighted,ListManager::locationLists[ListManager::iActiveList]->nLength);

			if(nWeighted > 0) {

				//Allocate permanent array:
				weightings.resize(nWeighted);
				locationIndex.resize(nWeighted);
				int nActive = 0;
				for(int i=0; i<nElements; i++) {
					//Check if location is actually populated:
					LocationMultiHost *loc = world->locationArray[i];
					if(loc != NULL) {
						double testWeight = bByPresence + bByDensity*loc->getCoverage(-1) + bUseSusceptibility*loc->getSusceptibilityWeightedCoverage(-1);
						if(testWeight) {
							locationIndex[nActive] = i;
							weightings[nActive] = testWeight;
							nActive++;
						}
					}
				}
			}

		} else {

			int nElements = world->header->nCols*world->header->nRows;

			vector<double> tempArray(nElements);

			if (readRasterToArray(weightRasterName, &tempArray[0])) {

				//Establish weightings of all locations in landscape:
				nWeighted = 0;
				int nElements = world->header->nCols*world->header->nRows;
				for(int i=0; i<nElements; i++) {
					//Check if location is actually populated:
					LocationMultiHost *loc = world->locationArray[i];
					if(loc != NULL) {
						double testWeight = tempArray[i];
						if(testWeight > 0.0) {
							nWeighted++;
						}
					}
				}

				printk("nWeighted=%d, nActive=%d\n",nWeighted,ListManager::locationLists[ListManager::iActiveList]->nLength);

				if(nWeighted > 0) {

					//Allocate permanent array:
					weightings.resize(nWeighted);
					locationIndex.resize(nWeighted);
					int nActive = 0;
					for(int i=0; i<nElements; i++) {
						//Check if location is actually populated:
						LocationMultiHost *loc = world->locationArray[i];
						if(loc != NULL && tempArray[i] > 0.0) {
							locationIndex[nActive] = i;
							weightings[nActive] = tempArray[i];
							nActive++;
						}
					}
				}

			} else {
				reportReadError("ERROR: Specified use of raster weighted random starting locations. Unable to open file L_STARTWEIGHTING.txt\n");
			}


		}


		if(nWeighted <= 0) {
			//Weren't actually any places to survey:
			reportReadError("ERROR: No locations in landscape are possible to infect.\n");
		}

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterInitialInf::~InterInitialInf() {

}

int InterInitialInf::intervene() {
	//printf("INITIAL INFECTION!\n");

	//Pick a location on chosen weighting:
	//Work out how many samples are actually going to be taken:
	int nSampleLocations = min(nLocations,nWeighted);
	pSampleLocations.resize(nSampleLocations);

	RateTree *weightTree = new RateTree(nWeighted);

	//Submit all the weightings to the structure:
	for(int i=0; i<nWeighted; i++) {
		weightTree->submitRate_dumb(i,weightings[i]);
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
			pSampleLocations[nPick] = locationIndex[nIndex];

			nPick++;

			//Don't want to pick the same location twice:
			weightTree->submitRate(nIndex, 0.0);
		}
	}

	delete weightTree;

	//Cause Infections:
	for(int i=0; i<nSampleLocations; i++) {
		world->locationArray[pSampleLocations[i]]->causeInfection(1, -1);
	}


	timeNext += frequency;

	return 1;
}

int InterInitialInf::finalAction() {
	//printf("InterInitialInf::finalAction()\n");

	return 1;
}

void InterInitialInf::writeSummaryData(FILE *pFile) {

	//TODO: Output information on what was done where

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Caused %d infections\n\n", (int)pSampleLocations.size());

		fprintf(pFile, "Sample locations x y\n");
		for (size_t iSample = 0; iSample < pSampleLocations.size(); iSample++) {
			int index = pSampleLocations[iSample];
			int y = (int)index / (int)world->header->nCols; //Actually want integer division for first time ever
			int x = index - y * world->header->nCols;
			fprintf(pFile, "%d %d\n", x, y);
		}
		fprintf(pFile, "\n");

	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
