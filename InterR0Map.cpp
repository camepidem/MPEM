//InterR0Map.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "Population.h"
#include "SummaryDataManager.h"
#include "ListManager.h"
#include "RasterHeader.h"
#include "myRand.h"
#include "InterR0Map.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterR0Map::InterR0Map() {

	timeSpent = 0;
	InterName = "R0Map";
	
	setCurrentWorkingSubection(InterName, CM_ENSEMBLE);


	windowXmin = 0;
	windowXmax = world->header->nCols-1;
	windowYmin = 0;
	windowYmax = world->header->nRows-1;

	nInfect = 1;
	xPos = 0;
	yPos = 0;
	fixedStart = 0;

	//Read Data:
	enabled = 0;
	readValueToProperty("R0MapEnable",&enabled,-1, "[0,1]");

	if(enabled) {

		//Instead of arbitrary duplication, use simulation length:
		frequency = world->timeEnd - world->timeStart;

		outputFrequency = frequency;
		readValueToProperty("R0MapOutputFrequency",&outputFrequency,-2, ">0");

		timeFirst = world->timeStart + outputFrequency;
		timeNext = timeFirst;

		if(outputFrequency > 0.0) {
			double tTest = 0.0;
			nOutputs = 0;
			while(tTest < frequency) {
				tTest += outputFrequency;
				nOutputs++;
			}
		} else {
			reportReadError("ERROR: R0MapOutputFrequency <= 0.0\n");
		}

		nInfect = 1;
		readValueToProperty("R0MapRuns",&nInfect,-2, ">0");
		if(nInfect <= 0) {
			reportReadError("ERROR: R0MapRuns <=0, Read: R0MapRuns %d\n",nInfect);
		}


		iStartingPopulation = -1;//Start based weighted by HostDensity
		printToKeyFile("//A value of -1 means to start in a random subpopulation weighted by the host density of each population within the cell\n");
		readValueToProperty("R0MapStartingPopulation",&iStartingPopulation,-2, "[-1,#nPopulations#)");//Always start in population iStartingPop


		bGiveVariances = 1;
		readValueToProperty("R0MapGiveVariance",&bGiveVariances,-2, "[0,1]");

		bGiveMeanSquare = 0;
		readValueToProperty("R0MapGiveMeanSquare",&bGiveMeanSquare,-2, "[0,1]");

		if(bGiveMeanSquare) {
			bGiveVariances = 1;
		}


		currentIteration = 0;

		readValueToProperty("R0MapFixedStart",&fixedStart,-2, "[0,1]");

		if(fixedStart) {
			printk("Using fixed starting point for R0Map\n");

			xPos = 0;
			readValueToProperty("R0MapX",&xPos,3, "[0,#nCols#)");

			yPos = 0;
			readValueToProperty("R0MapY",&yPos,3, "[0,#nRows#)");

		}

		//Optional inoculate locations with uniform background rate
		//Gives waiting time for first infection:
		bUseUniformBackgroundInoculation = 0;
		readValueToProperty("R0MapUniformBackgroundInoculation",&bUseUniformBackgroundInoculation,-2, "[0,1]");

		if(bUseUniformBackgroundInoculation) {
			dUniformBackgroundRate = 1.0;
			readValueToProperty("R0MapUniformBackgroundRate",&dUniformBackgroundRate,3, ">0");

			//Error check rate:
			if(dUniformBackgroundRate > 0.0) {
				if(1.0/dUniformBackgroundRate > frequency) {
					printk("Warning: Minimum mean waiting time is longer than R0MapTime\n");
				}
			} else {
				reportReadError("ERROR: Uniform background rate negative\n");
			}


		}


		//Optional subwindow:
		if(bIsKeyMode() && world->header->nCols < 0) {
			windowXmax = 1;
			windowYmax = 1;
		}

		printToKeyFile("//Windows run over cells in a range from inclusive of the min value and inclusive of the max value\n");
		printToKeyFile("//e.g. Xmin = 0 Xmax = 1 would specify a column two cells wide containing only columns 0 and 1\n");
		int tempInt = windowXmin;
		if(readValueToProperty("R0MapWindowXmin",&tempInt,-2, "[0,#nCols#)")) {
			if(tempInt>=0 && tempInt<world->header->nCols) {
				windowXmin = tempInt;
				printk("R0MapWindowXmin=%d\n",tempInt);
			} else {
				printk("Warning: R0MapWindowXmin=%d outside valid range: 0 to %d\n",tempInt,(world->header->nCols-1));
			}
		}
		tempInt = windowXmax;
		if(readValueToProperty("R0MapWindowXmax",&tempInt,-2, "[1,#nCols#]")) {
			if(tempInt>=0 && tempInt<world->header->nCols) {
				windowXmax = tempInt;
				printk("R0MapWindowXmax=%d\n",tempInt);
			} else {
				printk("Warning: R0MapWindowXmax=%d outside valid range: 0 to %d\n",tempInt,world->header->nCols-1);
			}
		}
		tempInt = windowYmin;
		if(readValueToProperty("R0MapWindowYmin",&tempInt,-2, "[0,#nRows#)")) {
			if(tempInt>=0 && tempInt<world->header->nRows) {
				windowYmin = tempInt;
				printk("R0MapWindowYmin=%d\n",tempInt);
			} else {
				printk("Warning: R0MapWindowYmin=%d outside valid range: 0 to %d\n",tempInt,(world->header->nRows-1));
			}
		}
		tempInt = windowYmax;
		if(readValueToProperty("R0MapWindowYmax",&tempInt,-2, "[1,#nRows#]")) {
			if(tempInt>=0 && tempInt<world->header->nRows) {
				windowYmax = tempInt;
				printk("R0MapWindowYmax=%d\n",tempInt);
			} else {
				printk("Warning: R0MapWindowYmax=%d outside valid range: 0 to %d\n",tempInt,world->header->nRows-1);
			}
		}

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}


	if(enabled && !world->bGiveKeys) {

		//Set up intervention:

		//Just disable normal ending condition and standard data output
		world->interventionManager->requestDisableStandardInterventions(this, InterName, world->timeStart + frequency);

		timeNext = world->timeStart + outputFrequency;

		if(timeNext > world->timeStart + frequency) {
			timeNext = world->timeStart + frequency;
		}

		nSnapshot = 0;

		nDone = 0;
		pctDone = 0;

		dDone = 0.0;

		//Duration limit
		nTimeStart = clock_ms();

		//Set up storage arrays:
		allocateStorage();

		//Initialise to NODATA:
		resetStorage();

		//Check how many locations are going to be run:
		estimateTotalCost();

		nTimeLastOutput = clock_ms();

		//Ensure that everything is starting properly

		//Force total clean as have no Touched list initially:
		world->cleanWorld(1);

		

		//Create Output files:
		writeFileHeaders();

		//Initialise for first infection:
		currentLoc = NULL;
		nSnapshot = nOutputs;
		currentIteration = nInfect;

		if(!fixedStart) {
			xPos = windowXmin - 1;//Start one before as selectNext increments automatically
			yPos = windowYmin;

			selectNextLocation();

		} else {
			currentLoc = world->locationArray[xPos + yPos*world->header->nCols];
			nSnapshot = 0;
			currentIteration = 0;
		}

		if(currentLoc != NULL && yPos <= windowYmax) {
			//clean the world back to starting state
			world->cleanWorld(0);

			//Get host allocations in initial state:
			getStartingData();

			//cause new infection
			infectCurrentLoc();
		} else {
			//force end of simulation...
			world->run = 0;
			reportReadError("ERROR: R0Map found no suitable starting locations: Simulation Terminated\n");
		}


		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char *timestring = asctime(timeinfo);

		char szProgressFileName[N_MAXFNAMELEN];
		sprintf(szProgressFileName,"%sR0MapProgress.txt",szPrefixOutput());

		progressFileName = string(szProgressFileName);

		FILE *pR0ProgressFile;
		pR0ProgressFile = fopen(progressFileName.c_str(),"w");
		if(pR0ProgressFile) {
			fprintf(pR0ProgressFile,"R0Map Started: %s\n",timestring);
			fclose(pR0ProgressFile);
		} else {
			printk("\nERROR: R0 Progress unable to write file\n\n");
		}

	}
}

InterR0Map::~InterR0Map() {

}

int InterR0Map::intervene() {
	//printf("R0 MAP!\n");

	//Store current landscape status snapshot in results arrays
	int currentLocNo;

	if(fixedStart) {
		currentLocNo = nDone;
	} else {
		currentLocNo = xPos;
	}

	double dmgL, dmgH;

	//For each weighting:
	for(int iWeight=0; iWeight<nWeightings; iWeight++) {//TODO: Augment summary manager and then modify this to use new interface

		//For Populations:
		for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {

			if(world->time > world->timeStart + dWaitingTime) {
				//Locations: Damage is number of unique locations ever infected:
				dmgL = SummaryDataManager::getUniquePopulationsInfected(iWeight)[iPop];
				//Biomass: Damage is loss in susceptibles (nSStart-nSNow) + those that died and were reborn (nRem-nR)
				dmgH = pdSusStart[iWeight][iPop] - SummaryDataManager::getHostQuantities(iPop, iWeight)[Population::pGlobalPopStats[iPop]->iSusceptibleClass];//double(nSusStart-world->nGlobalS+(world->nGlobalRem-world->nGlobalR))/double(world->nMaxHosts);
				if(Population::pGlobalPopStats[iPop]->bTrackingNEverRemoved) {
					//Have to account for hosts that have died and been reborn:
					dmgH += SummaryDataManager::getHostQuantities(iPop, iWeight)[Population::pGlobalPopStats[iPop]->iNEverRemoved];
					if(Population::pGlobalPopStats[iPop]->iRemovedClass > 0) {
						//But don't want to double count those hosts in the removed class:
						dmgH -= SummaryDataManager::getHostQuantities(iPop, iWeight)[Population::pGlobalPopStats[iPop]->iRemovedClass];
					}
				}
			} else {
				dmgL = 0.0;
				dmgH = 0.0;
			}

			//Locations:
			nLocsArray[iWeight][nSnapshot][iPop][currentLocNo] += dmgL;

			//Biomass:
			nHostsArray[iWeight][nSnapshot][iPop][currentLocNo] += dmgH;

			if(bGiveVariances) {
				//Var(X) = E(X^2) - E(X)^2

				//Store squares, then at end subtract square of means
				nVarLocsArray[iWeight][nSnapshot][iPop][currentLocNo] += dmgL*dmgL;

				nVarHostsArray[iWeight][nSnapshot][iPop][currentLocNo] += dmgH*dmgH;
			}

		}

		//For Locations:
		if(world->time > world->timeStart + dWaitingTime) {
			//Locations: Damage is number of unique locations ever infected
			dmgL = SummaryDataManager::getUniqueLocationsInfected(iWeight);
		} else {
			dmgL = 0.0;
		}

		nLocsArray[iWeight][nSnapshot][PopulationCharacteristics::nPopulations][currentLocNo] += dmgL;
		if(bGiveVariances) {
			//Var(X) = E(X^2) - E(X)^2

			//Store squares, then at end subtract square of means
			nVarLocsArray[iWeight][nSnapshot][PopulationCharacteristics::nPopulations][currentLocNo] += dmgL*dmgL;
		}

	}

	//Update snapshot
	nSnapshot++;

	if(nSnapshot >= nOutputs) {

		nSnapshot = 0;

		timeNext = world->timeStart + outputFrequency;

		if(timeNext > world->timeStart + frequency) {
			timeNext = world->timeStart + frequency;
		}

		//Increment location:
		if(fixedStart) {

			//Progress Bookkeeping:
			nDone++;
			dDone += currentLoc->getCoverage(-1);

			//Finished?:
			if(nDone >= nInfect) {
				currentLoc = NULL;
			}

		} else {
			//Increment iteration:
			currentIteration++;

			//Move on to new location:
			//(once we have done enough replicates on current location)
			if(currentIteration >= nInfect) {
				//Reset iteration counter:
				currentIteration = 0;

				//If we actually finished a location, progress Bookkeeping:
				nDone++;
				dDone += currentLoc->getCoverage(-1);

				selectNextLocation();

			}

		}

		//Record Progress:
		//Progress reporting:
		double proportionDone = double(nDone) / double(nTotal);
		int newPct = int(100.0*proportionDone);
		int tNow = clock_ms();

		if (newPct > pctDone || (tNow - nTimeLastOutput) > 60000) {

			if (newPct > pctDone) {
				pctDone = newPct;
			}

			double tElapsedMiliSeconds = tNow - nTimeStart;

			int tRemainingSeconds = 1000;
			if (proportionDone > 0.0) {
				tRemainingSeconds = int(ceil(0.001 * tElapsedMiliSeconds * (1.0 - proportionDone) / (proportionDone)));
			}

			double averageTimePerRun = 0.001 * tElapsedMiliSeconds / double(nDone);

			//May not have reached a new percentage marker, but if more than 1 minute has elapsed since last progress output, give some information:
			printToFileAndScreen(progressFileName.c_str(), 1, "Runs: %d done of %d total. Completion: %.2f%% Average time per run(s): %.2f Elapsed(s): %d ETR(s): %d\n", nDone, nTotal, 100.0*proportionDone, averageTimePerRun, int(0.001*tElapsedMiliSeconds), tRemainingSeconds);

			nTimeLastOutput = tNow;
		}

		//Ending condition:
		if(currentLoc != NULL) {
			//Clean the world back to starting state
			world->cleanWorld(0);

			//Reset all other interventions:
			world->interventionManager->resetInterventions();

			//Cause new infection
			infectCurrentLoc();

		} else {
			//Force end of simulation...
			world->run = 0;
			printk("R0 Simulation Ended\n");
		}

	} else {

		timeNext += outputFrequency;

		if(timeNext > world->timeStart + frequency) {
			timeNext = world->timeStart + frequency;
		}

	}

	return 1;
}

int InterR0Map::infectCurrentLoc() {

	int iPopToStart = 0;

	if(bUseUniformBackgroundInoculation) {
		//Generate a time of first infection for currentLoc based off uniform background rate:
		//TODO: Maybe allow properly weighted selection over all populations?
		double dRate = dUniformBackgroundRate*currentLoc->getSusceptibilityWeightedCoverage(iStartingPopulation);
		dWaitingTime = dExponentialVariate(dRate);

		//Start simulation after waiting time:
		world->time = world->timeStart + dWaitingTime;
	} else {
		dWaitingTime = 0.0;
	}

	//Select starting population:
	if(iStartingPopulation >= 0) {
		iPopToStart = iStartingPopulation;
	} else {
		//Pick starting population weighted by susceptibility:
		double dTotalCoverage = currentLoc->getSusceptibilityWeightedCoverage(-1);
		double dCoverage = dUniformRandom()*dTotalCoverage;
		double dCumulativeCoverage = 0.0;

		for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {
			dCumulativeCoverage += currentLoc->getSusceptibilityWeightedCoverage(iPop);
			if(dCumulativeCoverage >= dCoverage) {
				iPopToStart = iPop;
				break;
			}
		}

	}

	currentLoc->causeInfection(1, iPopToStart);

	return 1;
}

int InterR0Map::finalAction() {
	//printf("InterR0Map::finalAction()\n");

	//print out maps:
	if(enabled) {

		if(fixedStart) {
			writeDataFixed();
		}

		//Correct the world end time:
		//world->timeEnd = frequency;

	}

	return 1;
}

int InterR0Map::inBounds(LocationMultiHost *loc) {
	//check if the location is within the bounds of the sub rectangle for the R0 simulation

	//check xmin <= X <= xmax
	if(loc->xPos < windowXmin || loc->xPos > windowXmax) {
		return 0;
	}

	//check ymin <= Y <= ymax
	if(loc->yPos < windowYmin || loc->yPos > windowYmax) {
		return 0;
	}

	return 1;
}

void InterR0Map::allocateStorage() {

	nWeightings = SummaryDataManager::getNWeightings();

	int nElements;
	if(fixedStart) {
		nElements = nInfect;
	} else {
		nElements = world->header->nCols;
	}

	//For each weighting:
	nLocsArray.resize(nWeightings);
	nHostsArray.resize(nWeightings);

	if(bGiveVariances) {
		nVarLocsArray.resize(nWeightings);
		nVarHostsArray.resize(nWeightings);
	}

	//Starting data allocation:
	pdSusStart.resize(nWeightings);
	pnPlaceSusStart.resize(nWeightings);

	for(int iWeight=0; iWeight<nWeightings; iWeight++) {

		//Starting data allocation:
		pdSusStart[iWeight].resize(PopulationCharacteristics::nPopulations);
		pnPlaceSusStart[iWeight].resize(PopulationCharacteristics::nPopulations + 1);


		//For each output:
		nLocsArray[iWeight].resize(nOutputs);
		nHostsArray[iWeight].resize(nOutputs);

		if(bGiveVariances) {
			nVarLocsArray[iWeight].resize(nOutputs);
			nVarHostsArray[iWeight].resize(nOutputs);
		}

		for(int iOutput=0; iOutput<nOutputs; iOutput++) {

			nLocsArray[iWeight][iOutput].resize(PopulationCharacteristics::nPopulations + 1);
			nHostsArray[iWeight][iOutput].resize(PopulationCharacteristics::nPopulations + 1);

			if(bGiveVariances) {
				nVarLocsArray[iWeight][iOutput].resize(PopulationCharacteristics::nPopulations + 1);
				nVarHostsArray[iWeight][iOutput].resize(PopulationCharacteristics::nPopulations + 1);
			}

			//For each population:
			for(int iPop=0; iPop<PopulationCharacteristics::nPopulations + 1; iPop++) {

				//Allocate a row of data storage:
				nLocsArray[iWeight][iOutput][iPop].resize(nElements);
				nHostsArray[iWeight][iOutput][iPop].resize(nElements);

				if(bGiveVariances) {
					nVarLocsArray[iWeight][iOutput][iPop].resize(nElements);
					nVarHostsArray[iWeight][iOutput][iPop].resize(nElements);
				}
			}
		}

	}

	

	return;
}

void InterR0Map::resetStorage(int iElement) {

	int nElements;
	double dInitValue;
	if(fixedStart) {
		nElements = nInfect;
		dInitValue = 0.0;
	} else {
		nElements = world->header->nCols;
		dInitValue = double(world->header->NODATA);
	}

	int nStart = 0;
	int nEnd = nElements;
	if(iElement >=0) {
		nStart = iElement;
		nEnd = iElement + 1;
		dInitValue = 0.0;
	}

	//For each weighting:
	for(int iWeight=0; iWeight<nWeightings; iWeight++) {

		//For each output:
		for(int iOutput=0; iOutput<nOutputs; iOutput++) {

			//For each Population + Locations:
			for(int iPop=0; iPop<PopulationCharacteristics::nPopulations + 1; iPop++) {

				//Set the storage data to NODATA:
				double *pL = &nLocsArray[iWeight][iOutput][iPop][0];
				double *pH = &nHostsArray[iWeight][iOutput][iPop][0];

				for(int k=nStart; k<nEnd; k++) {
					pL[k] = dInitValue;
					pH[k] = dInitValue;
				}

				if(bGiveVariances) {
					double *pVL = &nVarLocsArray[iWeight][iOutput][iPop][0];
					double *pVH = &nVarHostsArray[iWeight][iOutput][iPop][0];

					for(int k=nStart; k<nEnd; k++) {
						pVL[k] = dInitValue;
						pVH[k] = dInitValue;
					}
				}
			}
		}
	}

	return;

}

void InterR0Map::estimateTotalCost() {

	if(fixedStart) {
		//Check Specified starting location actually exists:
		if(world->locationArray[xPos+world->header->nCols*yPos] != NULL) {
			nTotal = nInfect;
			dTotal = nInfect*world->locationArray[xPos+world->header->nCols*yPos]->getCoverage(-1);
		} else {
			printk("Specified R0Map starting location unpopulated!\n");
			//Disable intervention:
			nTotal = 0;
			dTotal = 0.0;
			frequency = world->timeEnd + 1e30;
			timeNext = world->timeEnd + 1e30;
		}
	} else {
		nTotal = 0;
		dTotal = 0.0;
		ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
		LocationMultiHost *LocIterator;
		while(pNode != NULL) {
			LocIterator = pNode->data;
			if(inBounds(LocIterator)) {
				nTotal++;
				dTotal += LocIterator->getCoverage(-1);
			}
			pNode = pNode->pNext;
		}
	}

	return;
}

void InterR0Map::getStartingData() {

	for(int iWeight=0; iWeight<nWeightings; iWeight++) {

		for (int iPop = 0; iPop<PopulationCharacteristics::nPopulations; iPop++) {
			pdSusStart[iWeight][iPop] = SummaryDataManager::getHostQuantities(iPop, iWeight)[0];
			pnPlaceSusStart[iWeight][iPop] = SummaryDataManager::getCurrentPopulationsSusceptible(iWeight)[iPop];
		}

		pnPlaceSusStart[iWeight][PopulationCharacteristics::nPopulations] = SummaryDataManager::getCurrentLocationsSusceptible(iWeight);

	}

	return;
}

void InterR0Map::selectNextLocation() {

	//Failure to select a valid location terminates the simulation
	currentLoc = NULL;

	while(currentLoc == NULL && yPos <= windowYmax) {
		//Try next location:
		xPos++;

		if(xPos > windowXmax) {
			//Have just completed a full row:
			writeDataLine();

			resetStorage();

			xPos = windowXmin;//Gets incremented before we select one:
			yPos++;
		}

		if(yPos <= windowYmax) {
			currentLoc = world->locationArray[xPos + yPos*world->header->nCols];
			if(currentLoc != NULL && inBounds(currentLoc)) {
				//Set all storage values to zero:
				resetStorage(xPos);

				nSnapshot = 0;
				currentIteration = 0;
			} else {
				//Set to null to force move on next time:
				currentLoc = NULL;
				nSnapshot = nOutputs;
				currentIteration = nInfect;
			}
		}

	}

	return;

}


void InterR0Map::writeFileHeaders() {

	if(!fixedStart) {
		char fName[_N_MAX_STATIC_BUFF_LEN];

		char szWeight[_N_MAX_STATIC_BUFF_LEN];
		for(int iWeight=0; iWeight<nWeightings; iWeight++) {
			
			if(iWeight > 0) {
				sprintf(szWeight, "_Weight_%d", (iWeight-1));
			} else {
				sprintf(szWeight, "%s", "");
			}

			double snapshotTime = world->timeStart + outputFrequency;
			
			for(int iOutput=0; iOutput<nOutputs; iOutput++) {

				if(snapshotTime > world->timeStart + frequency) {
					snapshotTime = world->timeStart + frequency;
				}

				//Population Level:
				for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {

					//Biomass
					sprintf(fName,"%sR0Hosts_Population_%d_%.6f%s_%d%s",szPrefixOutput(),iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);
					writeRasterFileHeaderRestricted(fName,windowXmin,windowXmax,windowYmin,windowYmax);

					//Locations:
					sprintf(fName,"%sR0Locations_Population_%d_%.6f%s_%d%s",szPrefixOutput(),iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);
					writeRasterFileHeaderRestricted(fName,windowXmin,windowXmax,windowYmin,windowYmax);

					if(bGiveVariances) {

						char *szMoment;

						if(!bGiveMeanSquare) {
							szMoment = "Variance";
						} else {
							szMoment = "MeanSquare";
						}

						sprintf(fName,"%sR0Hosts%s_Population_%d_%.6f%s_%d%s",szPrefixOutput(),szMoment,iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);
						writeRasterFileHeaderRestricted(fName,windowXmin,windowXmax,windowYmin,windowYmax);

						sprintf(fName,"%sR0Locations%s_Population_%d_%.6f%s_%d%s",szPrefixOutput(),szMoment,iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);
						writeRasterFileHeaderRestricted(fName,windowXmin,windowXmax,windowYmin,windowYmax);

					}
				}
				
				//Location Level:
				//Locations:
				if(PopulationCharacteristics::nPopulations > 1) {
					sprintf(fName,"%sR0Locations_Location_%.6f%s_%d%s",szPrefixOutput(),snapshotTime,szWeight,nInfect,world->szSuffixTextFile);
					writeRasterFileHeaderRestricted(fName,windowXmin,windowXmax,windowYmin,windowYmax);

					if(bGiveVariances) {

						char *szMoment;

						if(!bGiveMeanSquare) {
							szMoment = "Variance";
						} else {
							szMoment = "MeanSquare";
						}

						sprintf(fName,"%sR0Locations%s_Location_%.6f%s_%d%s",szPrefixOutput(),szMoment,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);
						writeRasterFileHeaderRestricted(fName,windowXmin,windowXmax,windowYmin,windowYmax);

					}
				}

				snapshotTime += outputFrequency;

			}

		}
	}
	return;
}

void InterR0Map::writeDataLine() {

	if(yPos < windowYmin || yPos > windowYmax) {
		//Outside the interesting window:
		return;
	}

	char fName[_N_MAX_STATIC_BUFF_LEN];

	double snapshotTime;
	double dRuns = double(nInfect);
	double *pData, *pMean;

	//For each weighting:
	for(int iWeight=0; iWeight<nWeightings; iWeight++) {

		snapshotTime = world->timeStart + outputFrequency;

		char szWeight[_N_MAX_STATIC_BUFF_LEN];

		if(iWeight > 0) {
			sprintf(szWeight, "_Weight_%d", (iWeight-1));
		} else {
			sprintf(szWeight, "%s", "");
		}

		for(int iOutput=0; iOutput<nOutputs; iOutput++) {

			if(snapshotTime > world->timeStart + frequency) {
				snapshotTime = world->timeStart + frequency;
			}

			//Population Level:
			for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {

				//Biomass
				sprintf(fName,"%sR0Hosts_Population_%d_%.6f%s_%d%s",szPrefixOutput(),iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

				pData = &nHostsArray[iWeight][iOutput][iPop][0];
				for(int k=0; k<world->header->nCols; k++) {
					if(pData[k] > 0.0) {
						pData[k] /= dRuns;
					}
				}

				writeArrayLine(fName,pData);


				//Locations:
				sprintf(fName,"%sR0Locations_Population_%d_%.6f%s_%d%s",szPrefixOutput(),iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

				pData = &nLocsArray[iWeight][iOutput][iPop][0];
				for(int k=0; k<world->header->nCols; k++) {
					if(pData[k] > 0.0) {
						pData[k] /= dRuns;
					}
				}

				writeArrayLine(fName,pData);


				if(bGiveVariances) {

					char *szMoment;

					if(!bGiveMeanSquare) {
						szMoment = "Variance";
					} else {
						szMoment = "MeanSquare";
					}

					sprintf(fName,"%sR0Hosts%s_Population_%d_%.6f%s_%d%s",szPrefixOutput(),szMoment,iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

					pData = &nVarHostsArray[iWeight][iOutput][iPop][0];
					pMean = &nHostsArray[iWeight][iOutput][iPop][0];

					if(!bGiveMeanSquare) {
						//Var(X) = E(X^2) - E(X)^2
						for(int k=0; k<world->header->nCols; k++) {
							//Don't divide NODATA values:
							if(pData[k] > 0.0) {
								pData[k] = pData[k]/dRuns - pMean[k]*pMean[k];
							}
						}
					} else {
						for(int k=0; k<world->header->nCols; k++) {
							//Don't divide NODATA values:
							if(pData[k] > 0.0) {
								pData[k] = pData[k]/dRuns;
							}
						}
					}


					writeArrayLine(fName,pData);

					sprintf(fName,"%sR0Locations%s_Population_%d_%.6f%s_%d%s",szPrefixOutput(),szMoment,iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

					pData = &nVarLocsArray[iWeight][iOutput][iPop][0];
					pMean = &nLocsArray[iWeight][iOutput][iPop][0];

					if(!bGiveMeanSquare) {
						//Var(X) = E(X^2) - E(X)^2
						for(int k=0; k<world->header->nCols; k++) {
							//Don't divide NODATA values:
							if(pData[k] > 0.0) {
								pData[k] = pData[k]/dRuns - pMean[k]*pMean[k];
							}
						}
					} else {
						for(int k=0; k<world->header->nCols; k++) {
							//Don't divide NODATA values:
							if(pData[k] > 0.0) {
								pData[k] = pData[k]/dRuns;
							}
						}
					}


					writeArrayLine(fName,pData);

				}
			}

			//Location Level:
			//Locations:
			if(PopulationCharacteristics::nPopulations > 1) {
				sprintf(fName,"%sR0Locations_Location_%.6f%s_%d%s",szPrefixOutput(),snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

				pData = &nLocsArray[iWeight][iOutput][PopulationCharacteristics::nPopulations][0];
				for(int k=0; k<world->header->nCols; k++) {
					if(pData[k] > 0.0) {
						pData[k] /= dRuns;
					}
				}

				writeArrayLine(fName,pData);

				if(bGiveVariances) {

					char *szMoment;

					if(!bGiveMeanSquare) {
						szMoment = "Variance";
					} else {
						szMoment = "MeanSquare";
					}

					sprintf(fName,"%sR0Locations%s_Location_%.6f%s_%d%s",szPrefixOutput(),szMoment,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

					pData = &nVarLocsArray[iWeight][iOutput][PopulationCharacteristics::nPopulations][0];
					pMean = &nLocsArray[iWeight][iOutput][PopulationCharacteristics::nPopulations][0];

					if(!bGiveMeanSquare) {
						//Var(X) = E(X^2) - E(X)^2
						for(int k=0; k<world->header->nCols; k++) {
							//Don't divide NODATA values:
							if(pData[k] > 0.0) {
								pData[k] = pData[k]/dRuns - pMean[k]*pMean[k];
							}
						}
					} else {
						for(int k=0; k<world->header->nCols; k++) {
							//Don't divide NODATA values:
							if(pData[k] > 0.0) {
								pData[k] = pData[k]/dRuns;
							}
						}
					}

					writeArrayLine(fName,pData);

				}

			}

			snapshotTime += outputFrequency;

		}

	}

	return;

}

void InterR0Map::writeDataFixed() {

	char fName[_N_MAX_STATIC_BUFF_LEN];

	double snapshotTime;
	double dRuns = double(nInfect);
	double *pData;
	double dMean, dMeanSquare, dVariance;

	//For each weighting:
	for(int iWeight=0; iWeight<nWeightings; iWeight++) {

		snapshotTime = world->timeStart + outputFrequency;

		char szWeight[_N_MAX_STATIC_BUFF_LEN];

		if(iWeight > 0) {
			sprintf(szWeight, "_Weight_%d", (iWeight-1));
		} else {
			sprintf(szWeight, "%s", "");
		}

		for(int iOutput=0; iOutput<nOutputs; iOutput++) {

			if(snapshotTime > world->timeStart + frequency) {
				snapshotTime = world->timeStart + frequency;
			}

			//Population Level:
			for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {

				//Biomass
				sprintf(fName,"%sR0Hosts_Population_%d_%.6f%s_%d%s",szPrefixOutput(),iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

				pData = &nHostsArray[iWeight][iOutput][iPop][0];

				dMean = 0.0;
				dMeanSquare = 0.0;
				for(int k=0; k<nInfect; k++) {
					dMean += pData[k];
					dMeanSquare += pData[k]*pData[k];
				}

				dMean /= nInfect;
				dMeanSquare /= nInfect;
				dVariance = dMeanSquare - dMean*dMean;

				//Need to make files again, as they were made with headers for the non fixed start mode by default:
				writeNewFile(fName);

				printToFileAndScreen(fName, 0, "Mean %f\n", dMean);
				printToFileAndScreen(fName, 0, "MeanSquare %f\n", dMeanSquare);
				printToFileAndScreen(fName, 0, "Variance %f\n", dVariance);

				writeTable(fName, nInfect, 1, pData);

				//Locations:
				sprintf(fName,"%sR0Locations_Population_%d_%.6f%s_%d%s",szPrefixOutput(),iPop,snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

				pData = &nLocsArray[iWeight][iOutput][iPop][0];
				dMean = 0.0;
				dMeanSquare = 0.0;
				for(int k=0; k<nInfect; k++) {
					dMean += pData[k];
					dMeanSquare += pData[k]*pData[k];
				}

				dMean /= nInfect;
				dMeanSquare /= nInfect;
				dVariance = dMeanSquare - dMean*dMean;

				//Need to make files again, as they were made with headers for the non fixed start mode by default:
				writeNewFile(fName);

				printToFileAndScreen(fName, 0, "Mean %f\n", dMean);
				printToFileAndScreen(fName, 0, "MeanSquare %f\n", dMeanSquare);
				printToFileAndScreen(fName, 0, "Variance %f\n", dVariance);

				writeTable(fName, nInfect, 1, pData);

			}

			//Location Level:
			if(PopulationCharacteristics::nPopulations > 1) {
				//Locations:
				sprintf(fName,"%sR0Locations_Location_%.6f%s_%d%s",szPrefixOutput(),snapshotTime,szWeight,nInfect,world->szSuffixTextFile);

				pData = &nLocsArray[iWeight][iOutput][PopulationCharacteristics::nPopulations][0];
				dMean = 0.0;
				dMeanSquare = 0.0;
				for(int k=0; k<nInfect; k++) {
					dMean += pData[k];
					dMeanSquare += pData[k]*pData[k];
				}

				dMean /= nInfect;
				dMeanSquare /= nInfect;
				dVariance = dMeanSquare - dMean*dMean;

				//Need to make files again, as they were made with headers for the non fixed start mode by default:
				writeNewFile(fName);

				printToFileAndScreen(fName, 0, "Mean %f\n", dMean);
				printToFileAndScreen(fName, 0, "MeanSquare %f\n", dMeanSquare);
				printToFileAndScreen(fName, 0, "Variance %f\n", dVariance);

				writeTable(fName, nInfect, 1, pData);
			}


			snapshotTime += outputFrequency;

		}

	}

	return;

}

void InterR0Map::writeArrayLine(char *szFileName, double *pArray) {

	FILE *pFile;

	char *szBuff = new char[_N_MAX_DYNAMIC_BUFF_LEN];

	pFile = fopen(szFileName,"a");
	if(pFile) {

		char *szPtr = szBuff;

		for(int i=windowXmin; i<=windowXmax; i++) {
			szPtr += sprintf(szPtr,"%.6f ",pArray[i]);
		}

		szPtr += sprintf(szPtr,"\n");

		fprintf(pFile,"%s", szBuff);

		fclose(pFile);
	} else {
		printf("ERROR: Unable to write file %s\n",szFileName);
	}

	delete [] szBuff;

	return;
}

void InterR0Map::writeArrayLine(char *szFileName, int *pArray) {

	FILE *pFile;

	char *szBuff = new char[_N_MAX_DYNAMIC_BUFF_LEN];

	pFile = fopen(szFileName,"a");
	if(pFile) {

		char *szPtr = szBuff;

		for(int i=windowXmin; i<=windowXmax; i++) {
			szPtr += sprintf(szPtr,"%d ",pArray[i]);
		}

		szPtr += sprintf(szPtr,"\n");

		fprintf(pFile,"%s", szBuff);

		fclose(pFile);
	} else {
		printf("ERROR: Unable to write file %s\n",szFileName);
	}

	delete [] szBuff;

	return;
}

void InterR0Map::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {

		fprintf(pFile, "Created Output\n");

		//Give data on window:
		//If a non trivial window was used:
		if(windowXmin != 0 || windowXmax != world->header->nCols-1 || windowYmin != 0 || windowYmax != world->header->nRows-1) {
			fprintf(pFile, "R0MapWindowXmin %d\n",windowXmin);
			fprintf(pFile, "R0MapWindowXmax %d\n",windowXmax);
			fprintf(pFile, "R0MapWindowYmin %d\n",windowYmin);
			fprintf(pFile, "R0MapWindowYmax %d\n",windowYmax);
		}

		if(nWeightings > 1) {
			fprintf(pFile, "Used %d weightings\n",nWeightings);
		}

		fprintf(pFile, "\n");

	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
