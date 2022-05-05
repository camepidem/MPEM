//InterRiskMap.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "InterEndSim.h"
#include "InterOutputDataDPC.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "ListManager.h"
#include "RasterHeader.h"
#include "InterRiskMap.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterRiskMap::InterRiskMap() {
	timeSpent = 0;
	InterName = "RiskMap";

	setCurrentWorkingSubection(InterName, CM_ENSEMBLE);

	frequency = 1e30;
	timeFirst = world->timeEnd + 1e30;
	timeNext = timeFirst;

	int tempInt = 0;
	enabled = 0;
	readValueToProperty("RiskMapEnable", &enabled,-1, "[0,1]");

	if(enabled) {

		//Read Data:
		//Instead of arbitrary duplication, use simulation length:
		frequency = world->timeEnd - world->timeStart;

		iInitialRunNo = world->runNo;

		outputFrequency = frequency;
		readValueToProperty("RiskMapOutputFrequency",&outputFrequency,-2, ">0");

		if(outputFrequency > 0.0) {
			double tTest = 0.0;
			nOutputs = 0;
			while(tTest < frequency) {
				tTest += outputFrequency;
				nOutputs++;
			}
		} else {
			reportReadError("ERROR: RiskMapOutputFrequency <= 0.0\n");
		}

		nTotal = 1;
		readValueToProperty("RiskMapRuns",&nTotal,2, ">0");

		nMaxDurationSeconds = 0;
		readValueToProperty("RiskMapMaxDurationSeconds", &nMaxDurationSeconds, -2, ">=0");

		if (nMaxDurationSeconds < 0) {
			reportReadError("ERROR: RiskMap max duration in seconds was negative, read: %d", nMaxDurationSeconds);
		}

		bGiveVariances = 0;
		readValueToProperty("RiskMapGiveVariance",&bGiveVariances,-2, "[0,1]");

		bGiveMeanSquare = 0;
		readValueToProperty("RiskMapGiveMeanSquare",&bGiveMeanSquare,-2, "[0,1]");

		if(bGiveMeanSquare) {
			bGiveVariances = 1;
		}

		//Data to track:
		bTrackProbInfection = 1;
		bOutputProbInfection = 1;
		bTrackProbSymptomatic = 1;
		bTrackSeverity = 1;
		bTrackTimeFirstInfection = 1;

		bTrackInoculumExport = 1;
		bTrackInoculumDeposition = 1;

		int tmpInt;

		tmpInt = 0;
		readValueToProperty("RiskMapDisableTrackProbabilityInfection",&tmpInt,-2, "[0,1]");
		if(tmpInt) {
			bTrackProbInfection = 0;
		}

		tmpInt = 0;
		readValueToProperty("RiskMapDisableTrackProbabilitySymptomatic",&tmpInt,-2, "[0,1]");
		if(tmpInt) {
			bTrackProbSymptomatic = 0;
		}

		tmpInt = 0;
		readValueToProperty("RiskMapDisableTrackSeverity",&tmpInt,-2, "[0,1]");
		if(tmpInt) {
			bTrackSeverity = 0;
		}

		tmpInt = 0;
		readValueToProperty("RiskMapDisableTrackTimeFirstInfection",&tmpInt,-2, "[0,1]");
		if(tmpInt) {
			bTrackTimeFirstInfection = 0;
		}

		tmpInt = 0;
		readValueToProperty("RiskMapDisableTrackInoculumDeposition",&tmpInt,-2, "[0,1]");
		if(tmpInt) {
			bTrackInoculumDeposition = 0;
		}

		tmpInt = 0;
		readValueToProperty("RiskMapDisableTrackInoculumExport",&tmpInt,-2, "[0,1]");
		if(tmpInt) {
			bTrackInoculumExport = 0;
		}

		if (!world->bTrackingInoculumDeposition()) {
			if (bTrackInoculumDeposition) {
				bTrackInoculumDeposition = 0;
				printk("Warning: RiskMap Unable to track inoculum deposition because config option TrackInoculumDeposition is disabled.\n");
			}
		}

		if (!world->bTrackingInoculumExport()) {
			if (bTrackInoculumExport) {
				bTrackInoculumExport = 0;
				printk("Warning: RiskMap Unable to track inoculum export because config option TrackInoculumExport is disabled.\n");
			}
		}

		if(!bTrackProbInfection) {
			bOutputProbInfection = 0;
			if(bTrackProbSymptomatic || bTrackTimeFirstInfection) {//Need to track PInfection if use either MeanSev or TimeFirstInf, but may not want to output it...
				bTrackProbInfection = 1;
			}
		}

		bAllowStandardOutput = 1;
		readValueToProperty("RiskMapAllowStandardOutput",&bAllowStandardOutput,-2, "[0,1]");

	}

	//Set up intervention:
	if(enabled && !world->bGiveKeys) {
		timeNext = world->timeStart + outputFrequency;

		if(timeNext > world->timeStart + frequency) {
			timeNext = world->timeStart + frequency;
		}

		nSnapshot = 0;


		if(bAllowStandardOutput) {
			//Prevent simulations ending prematurely:
			world->interventionManager->pInterEndSim->disable();
			//world->timeEnd += 1e30;
			//Stop DPC spamming to the screen:
			world->interventionManager->pInterDPC->makeQuiet();
		} else {
			//Just disable normal ending condition and standard data output
			world->interventionManager->requestDisableStandardInterventions(this, InterName, frequency);
		}


		//Set up result storage:
		//Use population internal data storage:
		nElementsTotalStorage = (bTrackProbInfection + bTrackProbSymptomatic + bTrackSeverity + bTrackInoculumExport + bTrackInoculumDeposition*PopulationCharacteristics::nPopulations*PopulationCharacteristics::nPopulations)*nOutputs + bTrackTimeFirstInfection;
		nElementsTotalStorage *= 1 + bGiveVariances;

		int iElementStart = 0;
		if(bTrackProbInfection) {
			iElementsProbInfection = iElementStart;
			iElementStart += nOutputs;
		} else {
			iElementsProbInfection = -1;
		}

		if(bTrackProbSymptomatic) {
			iElementsProbSymptomatic = iElementStart;
			iElementStart += nOutputs;
		} else {
			iElementsProbSymptomatic = -1;
		}

		if(bTrackSeverity) {
			iElementsMeanSeverity = iElementStart;
			iElementStart += nOutputs;
		} else {
			iElementsMeanSeverity= -1;
		}

		if(bTrackTimeFirstInfection) {
			iElementsTimeFirstInfection = iElementStart;
			iElementStart += 1;
		} else {
			iElementsTimeFirstInfection = -1;
		}

		if(bTrackInoculumExport) {
			iElementsInoculumExport = iElementStart;
			iElementStart += nOutputs;
		} else {
			iElementsInoculumExport = -1;
		}

		if(bTrackInoculumDeposition) {
			iElementsInoculumDeposition = iElementStart;
			iElementStart += nOutputs*PopulationCharacteristics::nPopulations*PopulationCharacteristics::nPopulations;
		} else {
			iElementsInoculumDeposition = -1;
		}

		ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
		LocationMultiHost *activeLocation;

		//Pass over all active locations
		while(pNode != NULL) {
			activeLocation = pNode->data;
			activeLocation->createDataStorage(nElementsTotalStorage);
			pNode = pNode->pNext;
		}

		//Iteration counters:
		nDone = 0;
		pctDone = 0;

		//Duration limit
		nTimeStart = clock_ms();

		//Progress reporting:
		nTimeLastOutput = clock_ms();

		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char *timestring = asctime(timeinfo);

		char szProgressFileName[N_MAXFNAMELEN];
		sprintf(szProgressFileName,"%sRiskMapProgress.txt",szPrefixOutput());

		progressFileName = string(szProgressFileName);

		FILE *pRiskMapProgressFile;
		pRiskMapProgressFile = fopen(progressFileName.c_str(),"w");
		if(pRiskMapProgressFile) {
			fprintf(pRiskMapProgressFile,"RiskMap Started: %s\n",timestring);
			fclose(pRiskMapProgressFile);
		} else {
			printk("\nERROR: Risk Map Progress unable to write file\n\n");
		}

	}
}

InterRiskMap::~InterRiskMap() {

}

int InterRiskMap::intervene() {
	//printf("Risk MAP!\n");

	//Store current landscape status snapshot in results arrays
	//Can use touched list to cut down on work:
	ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iTouchedList]->pFirst->pNext;
	LocationMultiHost *activeLocation;
	int iVarDataStart = nElementsTotalStorage/2;

	//Pass over all touched locations
	while(pNode != NULL) {
		activeLocation = pNode->data;

		for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {

			double severity = activeLocation->getEpidemiologicalMetric(i, PopulationCharacteristics::severity);

			if(bTrackProbInfection) {
				if(severity > 0.0) {
					activeLocation->incrementDataStorageElement(i, iElementsProbInfection + nSnapshot, +1.0);
					if(bGiveVariances) {
						activeLocation->incrementDataStorageElement(i, iVarDataStart + iElementsProbInfection + nSnapshot, +1.0);
					}
				}
			}



			if(bTrackProbSymptomatic) {

				double symptomaticFraction = activeLocation->getDetectableCoverageProportion(i);

				if(symptomaticFraction > 0.0) {
					activeLocation->incrementDataStorageElement(i, iElementsProbSymptomatic + nSnapshot, +1.0);
					if(bGiveVariances) {
						activeLocation->incrementDataStorageElement(i, iVarDataStart + iElementsProbSymptomatic + nSnapshot, +1.0);
					}
				}
			}

			if(bTrackSeverity) {
				if(severity > 0.0) {
					activeLocation->incrementDataStorageElement(i, iElementsMeanSeverity + nSnapshot, +severity);
					if(bGiveVariances) {
						activeLocation->incrementDataStorageElement(i, iVarDataStart + iElementsMeanSeverity + nSnapshot, +severity*severity);
					}
				}
			}

			if(bTrackTimeFirstInfection) {

				if(nSnapshot >= nOutputs-1) {
					double dFirstTime = activeLocation->getEpidemiologicalMetric(i, PopulationCharacteristics::timeFirstInfection);

					if(dFirstTime >= 0.0) {
						activeLocation->incrementDataStorageElement(i, iElementsTimeFirstInfection, +dFirstTime);
						if(bGiveVariances) {
							activeLocation->incrementDataStorageElement(i, iVarDataStart + iElementsTimeFirstInfection, +dFirstTime*dFirstTime);
						}
					}
				}
			}

			if(bTrackInoculumExport) {

				double dExport = activeLocation->getEpidemiologicalMetric(i, PopulationCharacteristics::inoculumExport);

				activeLocation->incrementDataStorageElement(i, iElementsInoculumExport + nSnapshot, + dExport);
				if(bGiveVariances) {
					activeLocation->incrementDataStorageElement(i, iVarDataStart + iElementsInoculumExport + nSnapshot, +dExport*dExport);
				}
			}

			if(bTrackInoculumDeposition) {

				for(int iSourcePop=0; iSourcePop<PopulationCharacteristics::nPopulations; iSourcePop++) {

					double dImport = activeLocation->getEpidemiologicalMetric(i, PopulationCharacteristics::inoculumDeposition + iSourcePop);

					activeLocation->incrementDataStorageElement(i, iElementsInoculumDeposition + iSourcePop + i*PopulationCharacteristics::nPopulations + nSnapshot*PopulationCharacteristics::nPopulations*PopulationCharacteristics::nPopulations, + dImport);
					if(bGiveVariances) {
						activeLocation->incrementDataStorageElement(i, iVarDataStart + iElementsInoculumDeposition + iSourcePop + i*PopulationCharacteristics::nPopulations + nSnapshot*PopulationCharacteristics::nPopulations*PopulationCharacteristics::nPopulations, +dImport*dImport);
					}

				}
			}

		}

		pNode = pNode->pNext;
	}

	//Completed snapshot:
	nSnapshot++;


	if(nSnapshot >= nOutputs) {
		//Have finished this run:

		//Restart data gathering:
		nSnapshot = 0;

		timeNext = world->timeStart + outputFrequency;

		if(timeNext > world->timeStart + frequency) {
			timeNext = world->timeStart + frequency;
		}

		//Update number complete:
		nDone++;

		bool bTimeLimitExceeded = nMaxDurationSeconds > 0 && clock_ms() > (nMaxDurationSeconds * 1000);

		if(nDone < nTotal && !bTimeLimitExceeded) {

			//Force final data outputs:
			for (int iInter = world->interventionManager->iInterDataExtractionStart; iInter <= world->interventionManager->iInterDataExtractionEnd; iInter++) {
				world->interventionManager->interventions[iInter]->performFinalAction();
			}

			//clean landscape using touched list ala R0Map
			world->cleanWorld(0,1,1);

			//Flag landscape needs to do full recalculation:
			world->bFlagNeedsFullRecalculation();

			//Reset all other interventions:
			world->interventionManager->resetInterventions();
		}

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
		
		if (nDone >= nTotal || bTimeLimitExceeded) {
			//force end of simulation...
			world->run = 0;
			printk("RiskMap Simulation Ended");
			if (bTimeLimitExceeded) {
				nTotal = nDone; //We need to log that we haven't actually done as many as we thought we would
				printk(" by exceeding time limit of %ds", nMaxDurationSeconds);
			}
			printk("\n");
		}

	} else {

		//Continue run to next output point:
		timeNext += outputFrequency;

		if(timeNext > world->timeStart + frequency) {
			timeNext = world->timeStart + frequency;
		}

	}

	return 1;
}

int InterRiskMap::reset() {

	return enabled;

}

int InterRiskMap::finalAction() {
	//printf("InterRiskMap::finalAction()\n");

	if(enabled) {

		//Unfudge output logging:
		auto currentRunNo = world->runNo;
		world->setRunNo(iInitialRunNo);

		double snapshotTime = world->timeStart + outputFrequency;

		//Create temporary output buffer rasters:
		int nElements = world->header->nRows*world->header->nCols;
		vector<double> pMeanArray(nElements);
		vector<double> pMomentArray;
		if(bGiveVariances) {
			pMomentArray.resize(nElements);
		}

		char fName[_N_MAX_STATIC_BUFF_LEN];

		ListNode<LocationMultiHost> *pNode;
		LocationMultiHost *activeLocation;
		int iVarDataStart = nElementsTotalStorage/2;
		int nPopulations = PopulationCharacteristics::nPopulations;
		double fTotal = double(nTotal);

		//For each snapshot
		for(int iOutput=0; iOutput<nOutputs; iOutput++) {

			if(snapshotTime > world->timeStart + frequency) {
				snapshotTime = world->timeStart + frequency;
			}

			//For each population:
			for(int iPopulation=0; iPopulation<nPopulations; iPopulation++) {

				//Clean arrays: (Only needs to be done once per population, not between types of output as will always overwrite output from previous category)
				for(int k=0; k<nElements; k++) {
					pMeanArray[k] = world->header->NODATA;
				}

				if(bGiveVariances) {
					for(int k=0; k<nElements; k++) {
						pMomentArray[k] = world->header->NODATA;
					}
				}

				if(bTrackProbInfection) {

					sprintf(fName,"%sProbabilityInfection_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

					//Pass over all active Populations:
					pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
					while(pNode != NULL) {
						activeLocation = pNode->data;

						double dVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbInfection + iOutput);
						pMeanArray[activeLocation->xPos + activeLocation->yPos*world->header->nCols] = dVal/fTotal;

						pNode = pNode->pNext;
					}

					if(bOutputProbInfection) {
						writeRaster(fName, &pMeanArray[0]);
					}

					if(bGiveVariances) {

						sprintf(fName,"%sProbabilityInfectionVariance_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

						if(bGiveMeanSquare) {
							sprintf(fName,"%sProbabilityInfectionMeanSquare_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);
						}

						pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
						if(!bGiveMeanSquare) {
							//Var(X) = E(X^2) - E(X)^2
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsProbInfection + iOutput);
								double dMean = pMeanArray[iIndex];
								pMomentArray[iIndex] = dVal/fTotal - dMean*dMean;

								pNode = pNode->pNext;
							}
						} else {
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsProbInfection + iOutput);
								pMomentArray[iIndex] = dVal/fTotal;

								pNode = pNode->pNext;
							}
						}

						if(bOutputProbInfection) {
							writeRaster(fName, &pMomentArray[0]);
						}

					}

				}


				if(bTrackProbSymptomatic) {

					sprintf(fName,"%sProbabilitySymptomatic_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

					//Pass over all active Populations:
					pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
					while(pNode != NULL) {
						activeLocation = pNode->data;

						double dVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbSymptomatic + iOutput);
						pMeanArray[activeLocation->xPos + activeLocation->yPos*world->header->nCols] = dVal/fTotal;

						pNode = pNode->pNext;
					}

					writeRaster(fName, &pMeanArray[0]);

					if(bGiveVariances) {

						sprintf(fName,"%sProbabilitySymptomaticVariance_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

						if(bGiveMeanSquare) {
							sprintf(fName,"%sProbabilitySymptomaticMeanSquare_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);
						}

						pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
						if(!bGiveMeanSquare) {
							//Var(X) = E(X^2) - E(X)^2
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsProbSymptomatic + iOutput);
								double dMean = pMeanArray[iIndex];
								pMomentArray[iIndex] = dVal/fTotal - dMean*dMean;

								pNode = pNode->pNext;
							}
						} else {
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsProbSymptomatic + iOutput);
								pMomentArray[iIndex] = dVal/fTotal;

								pNode = pNode->pNext;
							}
						}

						writeRaster(fName, &pMomentArray[0]);

					}

				}

				if(bTrackSeverity) {

					sprintf(fName,"%sSeverityMean_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

					//Pass over all active Populations:
					pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
					while(pNode != NULL) {
						activeLocation = pNode->data;

						double dVal = activeLocation->getDataStorageElement(iPopulation, iElementsMeanSeverity + iOutput);
						double pVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbInfection + iOutput);//# of runs severity > 0
						if(pVal > 0.0) {
							dVal /= pVal;
						}
						pMeanArray[activeLocation->xPos + activeLocation->yPos*world->header->nCols] = dVal;

						pNode = pNode->pNext;
					}

					writeRaster(fName, &pMeanArray[0]);

					if(bGiveVariances) {

						sprintf(fName,"%sSeverityVariance_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

						if(bGiveMeanSquare) {
							sprintf(fName,"%sSeverityMeanSquare_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);
						}

						pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
						if(!bGiveMeanSquare) {
							//Var(X) = E(X^2) - E(X)^2
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsMeanSeverity + iOutput);
								double pVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbInfection + iOutput);//# of runs severity > 0
								double dMean = pMeanArray[iIndex];
								if(pVal > 0.0) {
									pMomentArray[iIndex] = dVal/pVal - dMean*dMean;
								} else {
									pMomentArray[iIndex] = 0.0;
								}

								pNode = pNode->pNext;
							}
						} else {
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsMeanSeverity + iOutput);
								double pVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbInfection + iOutput);//# of runs severity > 0

								if(pVal > 0.0) {
									pMomentArray[iIndex] = dVal/pVal;
								} else {
									pMomentArray[iIndex] = 0.0;
								}

								pNode = pNode->pNext;
							}
						}

						writeRaster(fName, &pMomentArray[0]);

					}

				}

				if(bTrackInoculumExport) {

					string sOutputMetricName = PopulationCharacteristics::vsEpidemiologicalMetricsNames[PopulationCharacteristics::inoculumExport];

					sprintf(fName,"%s%s_Mean_Population_%d_%.6f_%d%s",szPrefixOutput(),sOutputMetricName.c_str(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

					//Pass over all active Populations:
					pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
					while(pNode != NULL) {
						activeLocation = pNode->data;

						double dVal = activeLocation->getDataStorageElement(iPopulation, iElementsInoculumExport + iOutput);
						pMeanArray[activeLocation->xPos + activeLocation->yPos*world->header->nCols] = dVal/fTotal;

						pNode = pNode->pNext;
					}

					writeRaster(fName, &pMeanArray[0]);

					if(bGiveVariances) {

						sprintf(fName,"%s%s_Variance_Population_%d_%.6f_%d%s",szPrefixOutput(),sOutputMetricName.c_str(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

						if(bGiveMeanSquare) {
							sprintf(fName,"%s%s_MeanSquare_Population_%d_%.6f_%d%s",szPrefixOutput(),sOutputMetricName.c_str(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);
						}

						pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
						if(!bGiveMeanSquare) {
							//Var(X) = E(X^2) - E(X)^2
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsInoculumExport + iOutput);
								double pVal = fTotal;//# of runs
								double dMean = pMeanArray[iIndex];
								if(pVal > 0.0) {
									pMomentArray[iIndex] = dVal/pVal - dMean*dMean;
								} else {
									pMomentArray[iIndex] = 0.0;
								}

								pNode = pNode->pNext;
							}
						} else {
							while(pNode != NULL) {
								activeLocation = pNode->data;

								int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

								double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsInoculumExport + iOutput);
								double pVal = fTotal;//# of runs

								if(pVal > 0.0) {
									pMomentArray[iIndex] = dVal/pVal;
								} else {
									pMomentArray[iIndex] = 0.0;
								}

								pNode = pNode->pNext;
							}
						}

						writeRaster(fName, &pMomentArray[0]);

					}

				}


				if(bTrackInoculumDeposition) {


					for(int iSourcePop=0; iSourcePop<PopulationCharacteristics::nPopulations; iSourcePop++) {

						string sOutputMetricName = PopulationCharacteristics::vsEpidemiologicalMetricsNames[PopulationCharacteristics::inoculumDeposition + iSourcePop];

						sprintf(fName,"%s%s_Mean_Population_%d_%.6f_%d%s",szPrefixOutput(),sOutputMetricName.c_str(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

						//Pass over all active Populations:
						pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
						while(pNode != NULL) {
							activeLocation = pNode->data;

							double dVal = activeLocation->getDataStorageElement(iPopulation, iElementsInoculumDeposition + iSourcePop + iPopulation*PopulationCharacteristics::nPopulations + iOutput*PopulationCharacteristics::nPopulations*PopulationCharacteristics::nPopulations);
							pMeanArray[activeLocation->xPos + activeLocation->yPos*world->header->nCols] = dVal/fTotal;

							pNode = pNode->pNext;
						}

						writeRaster(fName, &pMeanArray[0]);

						if(bGiveVariances) {

							sprintf(fName,"%s%s_Variance_Population_%d_%.6f_%d%s",szPrefixOutput(),sOutputMetricName.c_str(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

							if(bGiveMeanSquare) {
								sprintf(fName,"%s%s_MeanSquare_Population_%d_%.6f_%d%s",szPrefixOutput(),sOutputMetricName.c_str(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);
							}

							pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
							if(!bGiveMeanSquare) {
								//Var(X) = E(X^2) - E(X)^2
								while(pNode != NULL) {
									activeLocation = pNode->data;

									int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

									double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsInoculumDeposition + iSourcePop + iPopulation*PopulationCharacteristics::nPopulations + iOutput*PopulationCharacteristics::nPopulations*PopulationCharacteristics::nPopulations);
									double pVal = fTotal;//# of runs
									double dMean = pMeanArray[iIndex];
									if(pVal > 0.0) {
										pMomentArray[iIndex] = dVal/pVal - dMean*dMean;
									} else {
										pMomentArray[iIndex] = 0.0;
									}

									pNode = pNode->pNext;
								}
							} else {
								while(pNode != NULL) {
									activeLocation = pNode->data;

									int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

									double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsInoculumDeposition + iSourcePop + iPopulation*PopulationCharacteristics::nPopulations + iOutput*PopulationCharacteristics::nPopulations*PopulationCharacteristics::nPopulations);
									double pVal = fTotal;//# of runs

									if(pVal > 0.0) {
										pMomentArray[iIndex] = dVal/pVal;
									} else {
										pMomentArray[iIndex] = 0.0;
									}

									pNode = pNode->pNext;
								}
							}

							writeRaster(fName, &pMomentArray[0]);

						}

					}


				}

			}

			snapshotTime += outputFrequency;

		}

		//Time first Infection:
		snapshotTime = world->timeStart + frequency;

		if(bTrackTimeFirstInfection) {

			for(int iPopulation=0; iPopulation<nPopulations; iPopulation++) {

				//Clean arrays: (Only needs to be done once per population, not between types of output as will always overwrite output from previous category)
				for(int k=0; k<nElements; k++) {
					pMeanArray[k] = world->header->NODATA;
				}

				if(bGiveVariances) {
					for(int k=0; k<nElements; k++) {
						pMomentArray[k] = world->header->NODATA;
					}
				}


				sprintf(fName,"%sTimeFirstInfectionMean_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

				//Pass over all active Populations:
				pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
				while(pNode != NULL) {
					activeLocation = pNode->data;

					double dVal = activeLocation->getDataStorageElement(iPopulation, iElementsTimeFirstInfection);
					double pVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbInfection + nOutputs - 1);//# of runs w/ severity > 0 _by_FINAL_time_ (important)
					if(pVal > 0.0) {
						dVal /= pVal;
						pMeanArray[activeLocation->xPos + activeLocation->yPos*world->header->nCols] = dVal;
					}

					pNode = pNode->pNext;
				}

				writeRaster(fName, &pMeanArray[0]);

				if(bGiveVariances) {

					sprintf(fName,"%sTimeFirstInfectionVariance_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);

					if(bGiveMeanSquare) {
						sprintf(fName,"%sTimeFirstInfectionMeanSquare_Population_%d_%.6f_%d%s",szPrefixOutput(),iPopulation,snapshotTime,nTotal,world->szSuffixTextFile);
					}

					pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;
					if(!bGiveMeanSquare) {
						//Var(X) = E(X^2) - E(X)^2
						while(pNode != NULL) {
							activeLocation = pNode->data;

							int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

							double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsTimeFirstInfection);
							double pVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbInfection);//# of runs severity > 0
							double dMean = pMeanArray[iIndex];
							if(pVal > 0.0) {
								pMomentArray[iIndex] = dVal/pVal - dMean*dMean;
							} else {
								pMomentArray[iIndex] = 0.0;
							}

							pNode = pNode->pNext;
						}
					} else {
						while(pNode != NULL) {
							activeLocation = pNode->data;

							int iIndex = activeLocation->xPos + activeLocation->yPos*world->header->nCols;

							double dVal = activeLocation->getDataStorageElement(iPopulation, iVarDataStart + iElementsTimeFirstInfection);
							double pVal = activeLocation->getDataStorageElement(iPopulation, iElementsProbInfection);//# of runs severity > 0

							if(pVal > 0.0) {
								pMomentArray[iIndex] = dVal/pVal;
							} else {
								pMomentArray[iIndex] = 0.0;
							}

							pNode = pNode->pNext;
						}
					}

					writeRaster(fName, &pMomentArray[0]);

				}

			}

		}

		//Unfudge output logging:
		world->setRunNo(currentRunNo);

	}

	return 1;
}

void InterRiskMap::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Created Output\n");
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
