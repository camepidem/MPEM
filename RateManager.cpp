//RateManager.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "DispersalManager.h"
#include "RateStructure.h"
#include "RateCompositionRejection.h"
#include "RateTree.h"
#include "RateTreeInt.h"
#include "RateIntervals.h"
#include "RateSum.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "Intervention.h"
#include "DispersalKernel.h"
#include "RateConstantHistory.h"
#include "RateManager.h"

#pragma warning(disable : 4996)

Landscape *RateManager::world;
vector<vector<double>> RateManager::pdRateConstantsEffective;
vector<vector<double>> RateManager::pdRateConstantsScaleFactors;
vector<vector<double>> RateManager::pdRateConstantsDefault;

int RateManager::bStandardSpores;

RateManager::RateStructureType RateManager::rsRateStructureToUseForRateSLoss;
RateManager::RateStructureType RateManager::rsRateStructureToUseForStandards;

const char *RateManager::RateStructureTypeNames[] = {"Automatic", "Sum", "Intervals", "Tree", "CR"};

RateManager::RateManager() {

	//Memory:
	eventRatesExternal			= NULL;

	setCurrentWorkingSection("EVENT MANAGER");

	//Switch between different types of structure:
	rsRateStructureToUseForRateSLoss = TREE;
	rsRateStructureToUseForStandards = TREE;

	int iForceRatesSLoss = AUTO;
	int iForceRatesStandards = AUTO;

	//Normalisation constant for relative cost of operations in each structure
	double switchFactor = 1.0;

	setCurrentWorkingSubection("Rate Structures", CM_APPLICATION);

	char szKey[_N_MAX_STATIC_BUFF_LEN];

	//Valid rate structure list:
	char sTypeList[_N_MAX_STATIC_BUFF_LEN];
	sprintf(sTypeList, "{");
	for(int iRSN=0; iRSN<RATESTRUCTURE_MAX; iRSN++) {
		if(iRSN>0) {
			strcat(sTypeList, ",");
		}
		strcat(sTypeList, RateStructureTypeNames[iRSN]);
	}
	strcat(sTypeList, "}");
	
	char sForceRate[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szKey, "RatesForceRateStructureInfection");
	sprintf(sForceRate, "Automatic");
	if(readValueToProperty(szKey, sForceRate, -1, sTypeList)) {
		for(int iRSN=0; iRSN<RATESTRUCTURE_MAX; iRSN++) {
			if(strcmp(sForceRate, RateStructureTypeNames[iRSN]) == 0) {
				iForceRatesSLoss = iRSN;
			}
		}
		if(iForceRatesSLoss < 0) {
			reportReadError("ERROR: could not determine type of rate structure from '%s'. Valid types are: %s\n", sForceRate, sTypeList);
		}
	}

	sprintf(szKey, "RatesForceRateStructureStandard");
	sprintf(sForceRate, "Automatic");
	if(readValueToProperty(szKey, sForceRate, -1, sTypeList)) {
		for(int iRSN=0; iRSN<RATESTRUCTURE_MAX; iRSN++) {
			if(strcmp(sForceRate, RateStructureTypeNames[iRSN]) == 0) {
				iForceRatesStandards = iRSN;
			}
		}
		if(iForceRatesStandards < 0) {
			reportReadError("ERROR: could not determine type of rate structure from '%s'. Valid types are: %s\n", sForceRate, sTypeList);
		}
	}

	bStandardSpores = 1;

	int bSporeMagnet = 0;
	readValueToProperty("RatesSporesAreMagnetic", &bSporeMagnet, -1, "[0,1]");
	if (bSporeMagnet) {
		bStandardSpores = 0;
	}

	//If giving keys do not attempt to initialise data:
	if(!world->bGiveKeys && !world->bReadingErrorFlag) {

		eventRates.resize(PopulationCharacteristics::nPopulations);

		for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {

			int nEvents = Population::pGlobalPopStats[iPop]->nEvents;

			eventRates[iPop].resize(nEvents);

			//Switching condition:
			if(iForceRatesSLoss == AUTO) {
				//Work
				double dWorkForStructures[RATESTRUCTURE_MAX];

				//TODO: Fit coefficients...

				//CR: C*(log2(maxRate))
				dWorkForStructures[COMPOSITIONREJECTION] = 1.0;

				//TREE: T*kernelArea*log_2(N)
				dWorkForStructures[TREE] = 1.0;

				//INTERVALS: iPop*sqrt(N)
				dWorkForStructures[INTERVALS] = 1.0;

				//SUM: S*N
				dWorkForStructures[SUM] = 1.0;


				rsRateStructureToUseForRateSLoss = TREE;
			} else {
				rsRateStructureToUseForRateSLoss = (RateStructureType)iForceRatesSLoss;
			}


			//Rate of S loss:
			for(int iSourcePop = Population::pGlobalPopStats[iPop]->iInfectionEventsStart; iSourcePop <= Population::pGlobalPopStats[iPop]->iInfectionEventsEnd; iSourcePop++) {
				switch(rsRateStructureToUseForRateSLoss) {
				case SUM:
					printk("Using sum rate structure for infection events\n");
					eventRates[iPop][iSourcePop] = new RateSum(Population::pGlobalPopStats[iPop]->nActiveCount);
					break;
				case INTERVALS:
					printk("Using interval rate structure for infection events\n");
					eventRates[iPop][iSourcePop] = new RateIntervals(Population::pGlobalPopStats[iPop]->nActiveCount);
					break;
				case TREE:
					printk("Using tree rate structure for infection events\n");
					eventRates[iPop][iSourcePop] = new RateTree(Population::pGlobalPopStats[iPop]->nActiveCount);
					break;
				case COMPOSITIONREJECTION:
					printk("Using CR rate structure for infection events\n");
					//TODO: Optimal choice of bounds: Lower can always be arbitrarily small, upper is bounded above by N^2 as long as sus and inf are <= 1.0 and there is no flat primary infection
					eventRates[iPop][iSourcePop] = new RateCompositionRejection(Population::pGlobalPopStats[iPop]->nActiveCount, 0.125, Population::pGlobalPopStats[iPop]->nMaxHosts*Population::pGlobalPopStats[iPop]->nMaxHosts);
					break;
				default:
					reportReadError("%s %i\n", "ERROR: Cannot find RateStructure for infection events: ", rsRateStructureToUseForRateSLoss);
					break;
				}
			}

			//TODO: Actually makes more sense for the rates of the standard events to be CR than anything else...
			//Switching condition:
			if(iForceRatesStandards == AUTO) {
				rsRateStructureToUseForStandards = TREE;
			} else {
				rsRateStructureToUseForStandards = (RateStructureType)iForceRatesStandards;
			}


			//Standard events:
			for(int iEvent = Population::pGlobalPopStats[iPop]->iStandardEventsStart; iEvent <= Population::pGlobalPopStats[iPop]->iStandardEventsEnd; ++iEvent) {
				//Can store rates in integer:
				//eventRates[iPop][iEvent] = new RateTreeInt(Population::pGlobalPopStats[iPop]->nActiveCount);
				switch(rsRateStructureToUseForStandards) {
				case SUM:
					printk("Using sum rate structure for standard events\n");
					eventRates[iPop][iEvent] = new RateSum(Population::pGlobalPopStats[iPop]->nActiveCount);
					break;
				case INTERVALS:
					printk("Using interval rate structure for standard events\n");
					eventRates[iPop][iEvent] = new RateIntervals(Population::pGlobalPopStats[iPop]->nActiveCount);
					break;
				case TREE:
					printk("Using tree rate structure for standard events\n");
					eventRates[iPop][iEvent] = new RateTreeInt(Population::pGlobalPopStats[iPop]->nActiveCount);
					break;
				case COMPOSITIONREJECTION:
					printk("Using CR rate structure for standard events\n");
					//TODO: Bounds: as these events are raw ints, lowest positive rate must be 1.0, highest rate is bounded above by N
					eventRates[iPop][iEvent] = new RateCompositionRejection(Population::pGlobalPopStats[iPop]->nActiveCount, 0.125, Population::pGlobalPopStats[iPop]->nMaxHosts);
					break;
				default:
					reportReadError("%s %i\n", "ERROR: Cannot find RateStructure for standard events: ", rsRateStructureToUseForStandards);
					break;
				}

			}

			//Virtual Sporulation:
			switch(rsRateStructureToUseForStandards) {
			case SUM:
				printk("Using sum rate structure for VS events\n");
				eventRates[iPop][Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent] = new RateSum(Population::pGlobalPopStats[iPop]->nActiveCount);
				break;
			case INTERVALS:
				printk("Using interval rate structure for VS events\n");
				eventRates[iPop][Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent] = new RateIntervals(Population::pGlobalPopStats[iPop]->nActiveCount);
				break;
			case TREE:
				printk("Using tree rate structure for VS events\n");
				eventRates[iPop][Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent] = new RateTree(Population::pGlobalPopStats[iPop]->nActiveCount);
				break;
			case COMPOSITIONREJECTION:
				printk("Using CR rate structure for VS events\n");
				//TODO: Bounds: Lowest possible rate arbitrarily small, bounded above by N given inf <= 1.0
				eventRates[iPop][Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent] = new RateCompositionRejection(Population::pGlobalPopStats[iPop]->nActiveCount, 0.125, Population::pGlobalPopStats[iPop]->nMaxHosts);
				break;
			default:
				reportReadError("%s %i\n", "ERROR: Cannot find RateStructure for VS events: ", rsRateStructureToUseForStandards);
				break;
			}

		}

		//External Events:
		nMaxEventsExternal = 32;//Reasonable number to start with so hopefully don't ever have to grow...
		nEventsExternal = 0;
		eventRatesExternal = new RateTree(nMaxEventsExternal);
		piAdditionalArguments.resize(nMaxEventsExternal);
		externalRateInterventions.resize(nMaxEventsExternal);
		for(int iA=0; iA<nMaxEventsExternal; iA++) {
			piAdditionalArguments[iA] = 0;
			externalRateInterventions[iA] = NULL;
		}

	}
}

RateManager::~RateManager() {

	if(eventRates.size() != 0) {
		for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
			for(int j=0; j<Population::pGlobalPopStats[i]->nEvents; j++) {
				delete eventRates[i][j];
			}
		}
	}

	if(eventRatesExternal != NULL) {
		delete eventRatesExternal;
		eventRatesExternal = NULL;
	}

}

int RateManager::setLandscape(Landscape *myWorld) {

	world = myWorld;

	return 1;
}

int RateManager::readRateConstants() {

	setCurrentWorkingSubection("Rate Constants", CM_MODEL);

	printk("Reading Master Rate Data: ");

	char szKeyName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szKeyName, "Rate_BackgroundInfection");

	double dTempBetaBackground = 0.0;
	readValueToProperty(szKeyName, &dTempBetaBackground, -1, ">=0");

	if(world->bGiveKeys) {
		//Give some fluffy description of the format for all the rate constants
		
		char *szTestModel1 = "SEIS";
		size_t length1 = strlen(szTestModel1);

		char *szTestModel2 = "SIR";
		size_t length2 = strlen(szTestModel2);

		printToFileAndScreen(world->szKeyFileName,0,"\n//Example Rate Constants for a %s-%s Model:\n\n",szTestModel1,szTestModel2);

		printToFileAndScreen(world->szKeyFileName,0,"// Rate_0_Sporulation\n");

		for(size_t i=1; i<length1-1; i++) {
			printToFileAndScreen(world->szKeyFileName,0,"// Rate_0_%cto%c\n",szTestModel1[i],szTestModel1[i+1]);
		}

		printToFileAndScreen(world->szKeyFileName,0,"\n// Rate_1_Sporulation\n");

		for(size_t i=1; i<length2-1; i++) {
			printToFileAndScreen(world->szKeyFileName,0,"// Rate_1_%cto%c\n",szTestModel2[i],szTestModel2[i+1]);
		}

		printToKeyFile(" ~#RateConstants#~ 1.0 ~f>0~\n");

	} else {
		
		//Allocate storage for rates
		pdRateConstantsEffective.resize(PopulationCharacteristics::nPopulations);
		pdRateConstantsScaleFactors.resize(PopulationCharacteristics::nPopulations);
		pdRateConstantsDefault.resize(PopulationCharacteristics::nPopulations);

		rateConstantHistories.resize(PopulationCharacteristics::nPopulations);

		for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
			int nEvents = Population::pGlobalPopStats[i]->nEvents;

			pdRateConstantsEffective[i].resize(nEvents);
			pdRateConstantsScaleFactors[i].resize(nEvents);
			pdRateConstantsDefault[i].resize(nEvents);

			for(int iEvent=0; iEvent<nEvents; iEvent++) {
				pdRateConstantsEffective[i][iEvent] = 0.0;
				pdRateConstantsScaleFactors[i][iEvent] = 1.0;
				pdRateConstantsDefault[i][iEvent] = 0.0;
			}

			rateConstantHistories[i].resize(nEvents);

		}

		//Actually read some rate constants:

		//Primary infection:
		if (dTempBetaBackground >= 0.0) {
			//Legal value, use it:
			changeRateConstantDefault(0, Population::pGlobalPopStats[0]->iPrimaryInfectionEvent, dTempBetaBackground); //Setting primary infection for any population sets it for all
			scaleRateConstantFromDefault(0, Population::pGlobalPopStats[0]->iPrimaryInfectionEvent, 1.0);
		} else {
			printk("Warning: Negative background infection rate, truncating to zero.\n");
			//TODO: For this and other rates, check are +ve...
		}


		for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations; iPop++) {
			int nEvents = Population::pGlobalPopStats[iPop]->nEvents;

			//Secondary Infection Rate:
			double dTempRateConstant = 0.0;
			getRateConstantNameFromPopAndEvent(iPop, iPop, szKeyName);
			readValueToProperty(szKeyName,&dTempRateConstant, 1, ">0");

			if (dTempRateConstant <= 0.0) {
				reportReadError("ERROR: Rate constant %s must be positive (read %.6f). Use Climate if setting rate to zero is required.\n", szKeyName, dTempRateConstant);
				return 0;
			}

			changeRateConstantDefault(iPop, iPop, dTempRateConstant);


			//Standard transition events:
			for(int iEvent = Population::pGlobalPopStats[iPop]->iStandardEventsStart; iEvent <= Population::pGlobalPopStats[iPop]->iStandardEventsEnd; ++iEvent) {
				getRateConstantNameFromPopAndEvent(iPop, iEvent, szKeyName);
				dTempRateConstant = 1.0;
				readValueToProperty(szKeyName, &dTempRateConstant, 1,">=0");//&pdRateConstantsDefault[iPop][iEvent]

				if (dTempRateConstant < 0.0) {
					reportReadError("ERROR: Rate constant %s must be non-negative (read %.6f).\n", szKeyName, dTempRateConstant);
					return 0;
				}

				changeRateConstantDefault(iPop, iEvent, dTempRateConstant);
			}

			//Set current rate constants from defaults and intialise scale factors:
			//Infection events:
			scaleRateConstantFromDefault(iPop, iPop, 1.0);
			//Standard events:
			for(int iEvent = Population::pGlobalPopStats[iPop]->iStandardEventsStart; iEvent <= Population::pGlobalPopStats[iPop]->iStandardEventsEnd; ++iEvent) {
				scaleRateConstantFromDefault(iPop, iEvent, 1.0);
			}

		}

	}

	return 1;

}

//TODO: These functions should live inside the relevant popStats objects
int RateManager::getPopAndEventFromRateConstantName(char *szConstName, int &iPop, int &iEventType) {

	char szTestConstName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szTestConstName, "%s", szConstName);
	makeUpper(szTestConstName);

	char szTestKey[_N_MAX_STATIC_BUFF_LEN];

	if(strcmp("NULL_RATE", szTestConstName) == 0) {
		return 0;
	}

	if (strcmp("RATE_BACKGROUNDINFECTION", szTestConstName) == 0) {
		iPop = 0;
		iEventType = Population::pGlobalPopStats[0]->iPrimaryInfectionEvent;
		return 1;
	}

	//Loop over and compare...
	for(int iTestPop=0; iTestPop<PopulationCharacteristics::nPopulations; iTestPop++) {
		int nEvents = Population::pGlobalPopStats[iTestPop]->nEvents;
		for(int iTestEvent=0; iTestEvent<nEvents; iTestEvent++) {
			getRateConstantNameFromPopAndEvent(iTestPop, iTestEvent, szTestKey);
			makeUpper(szTestKey);
			if(strcmp(szTestKey, szTestConstName) == 0) {
				iPop = iTestPop;
				iEventType = iTestEvent;
				return 1;
			}
		}
	}

	return 0;

}

//TODO: These functions should live inside the relevant popStats objects
int RateManager::getRateConstantNameFromPopAndEvent(int iPop, int iEventType, char *szConstName) {

	int nPopulations = PopulationCharacteristics::nPopulations;
	if(iPop >= 0 && iPop < nPopulations) {
		if(iEventType >= Population::pGlobalPopStats[iPop]->iInfectionEventsStart && iEventType <= Population::pGlobalPopStats[iPop]->iInfectionEventsEnd) {

			if (iEventType == Population::pGlobalPopStats[iPop]->iPrimaryInfectionEvent) {
				sprintf(szConstName, "Rate_BackgroundInfection");
				return 1;
			}

			if(iEventType == iPop) {
				sprintf(szConstName,"Rate_%d_Sporulation",iPop);
				return 1;
			}

			sprintf(szConstName, "NULL_RATE");
			return 0;

		} else {
			int nEvents = Population::pGlobalPopStats[iPop]->nEvents;
			if(iEventType >= 0 && iEventType < nEvents) {
				int iClass = iEventType + 1 - Population::pGlobalPopStats[iPop]->iStandardEventsStart;
				sprintf(szConstName,"Rate_%d_%cto%c",iPop,Population::pGlobalPopStats[iPop]->szModelString[iClass],Population::pGlobalPopStats[iPop]->szModelString[iClass+1]);
				return 1;
			} else {
				return 0;
			}
		}
	}

	return 0;

}

double RateManager::getEffectiveRateConstantSLoss(int iPop, double dNewBeta) {
	
	return dNewBeta/PopulationCharacteristics::nMaxHosts;

}

double RateManager::getEffectiveRateConstantVirtualSporulation(int iPop, double dNewBeta) {
	
	return dNewBeta*DispersalManager::getKernel(iPop)->getVirtualSporulationProportion()/DispersalManager::dGetVSPatchScaling(iPop);

}

int RateManager::scaleRateConstantFromDefault(int iPop, int iEventType, double dFactor) {

	if(iEventType >= Population::pGlobalPopStats[iPop]->iInfectionEventsStart && iEventType <= Population::pGlobalPopStats[iPop]->iInfectionEventsEnd) {

		//Background Rate:
		if (iEventType == Population::pGlobalPopStats[iPop]->iPrimaryInfectionEvent) {
			//Setting primary infection for any population sets it for all
			for (int iPrimaryPop = 0; iPrimaryPop < PopulationCharacteristics::nPopulations; ++iPrimaryPop) {
				pdRateConstantsScaleFactors[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent] = dFactor;
				pdRateConstantsEffective[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent] = pdRateConstantsScaleFactors[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent] * pdRateConstantsDefault[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent];
				rateConstantHistories[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent].changeRateConstant(world->time, pdRateConstantsEffective[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent]);
			}
			return 1;
		}


		pdRateConstantsScaleFactors[iPop][iPop] = dFactor;

		//Also scale Virtual sporulation in case of changing beta:
		int iVS = Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent;
		pdRateConstantsScaleFactors[iPop][iVS] = dFactor;
		pdRateConstantsEffective[iPop][iVS] = pdRateConstantsScaleFactors[iPop][iVS]*pdRateConstantsDefault[iPop][iVS];

		rateConstantHistories[iPop][iVS].changeRateConstant(world->time, pdRateConstantsEffective[iPop][iVS]);

		//Update effective rate constant:
		for(int iReceptorPop=0; iReceptorPop<PopulationCharacteristics::nPopulations; iReceptorPop++) {
			pdRateConstantsEffective[iReceptorPop][iPop] = pdRateConstantsScaleFactors[iPop][iPop]*pdRateConstantsDefault[iPop][iPop];
			rateConstantHistories[iReceptorPop][iEventType].changeRateConstant(world->time, pdRateConstantsEffective[iReceptorPop][iPop]);
		}

	} else {
		pdRateConstantsScaleFactors[iPop][iEventType] = dFactor;
		
		//Update effective rate constant:
		pdRateConstantsEffective[iPop][iEventType] = pdRateConstantsScaleFactors[iPop][iEventType]*pdRateConstantsDefault[iPop][iEventType];
		rateConstantHistories[iPop][iEventType].changeRateConstant(world->time, pdRateConstantsEffective[iPop][iEventType]);
	}

	return 1;

}

int RateManager::changeRateConstantDefault(int iPop, int iEventType, double dNewRateConstant) {

	if (iEventType >= Population::pGlobalPopStats[iPop]->iInfectionEventsStart && iEventType <= Population::pGlobalPopStats[iPop]->iInfectionEventsEnd) {

		//Background Rate:
		if (iEventType == Population::pGlobalPopStats[iPop]->iPrimaryInfectionEvent) {
			////Setting primary infection for any population sets it for all
			for (int iPrimaryPop = 0; iPrimaryPop < PopulationCharacteristics::nPopulations; ++iPrimaryPop) {
				pdRateConstantsDefault[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent] = dNewRateConstant;
				pdRateConstantsEffective[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent] = pdRateConstantsScaleFactors[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent] * pdRateConstantsDefault[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent];
				rateConstantHistories[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent].changeRateConstant(world->time, pdRateConstantsEffective[iPrimaryPop][Population::pGlobalPopStats[iPrimaryPop]->iPrimaryInfectionEvent]);
			}
			return 1;
		}

		//Handle special beta scaling:
		int iVS = Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent;
		pdRateConstantsDefault[iPop][iEventType] = getEffectiveRateConstantSLoss(iPop, dNewRateConstant);
		//zero out unused rate constants, strictly unnecessary as they will (should) never be used:
		for (int iSourcePop = 0; iSourcePop < PopulationCharacteristics::nPopulations; iSourcePop++) {
			if (iSourcePop != iPop) {
				pdRateConstantsDefault[iPop][iSourcePop] = 0.0;
			}
		}
		pdRateConstantsDefault[iPop][iVS] = getEffectiveRateConstantVirtualSporulation(iPop, dNewRateConstant);

		//Update effective rate constants:
		for (int iReceptorPop = 0; iReceptorPop < PopulationCharacteristics::nPopulations; iReceptorPop++) {
			pdRateConstantsEffective[iReceptorPop][iPop] = pdRateConstantsScaleFactors[iPop][iPop] * pdRateConstantsDefault[iPop][iPop];
			rateConstantHistories[iReceptorPop][iPop].changeRateConstant(world->time, pdRateConstantsEffective[iReceptorPop][iPop]);
		}
		pdRateConstantsEffective[iPop][iVS] = pdRateConstantsScaleFactors[iPop][iVS] * pdRateConstantsDefault[iPop][iVS];
		rateConstantHistories[iPop][iVS].changeRateConstant(world->time, pdRateConstantsEffective[iPop][iVS]);

	} else {
		pdRateConstantsDefault[iPop][iEventType] = dNewRateConstant;

		//Update effective rate constant:
		pdRateConstantsEffective[iPop][iEventType] = pdRateConstantsScaleFactors[iPop][iEventType] * pdRateConstantsDefault[iPop][iEventType];
		rateConstantHistories[iPop][iEventType].changeRateConstant(world->time, pdRateConstantsEffective[iPop][iEventType]);
	}

	return 1;

}

//TODO: Why does this function exist? Hopefully just for the purpose of bookkeeping in the rateConstantHistory objects, otherwise terrifying...
int RateManager::refreshSporulationRateConstants() {

	for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations; iPop++) {

		double originalSporulationParameter = pdRateConstantsDefault[iPop][iPop] * PopulationCharacteristics::nMaxHosts;

		changeRateConstantDefault(iPop, iPop, originalSporulationParameter);

	}

	double originalPrimaryRate = pdRateConstantsDefault[0][Population::pGlobalPopStats[0]->iPrimaryInfectionEvent];

	changeRateConstantDefault(0, Population::pGlobalPopStats[0]->iPrimaryInfectionEvent, originalPrimaryRate);

	return 1;
}

double RateManager::getRateConstantAverage(int iPop, int iEventType, double dTimeStart, double dTimeEnd) {
	return rateConstantHistories[iPop][iEventType].getAverage(dTimeStart, dTimeEnd);
}

double RateManager::submitRate(int iPop, int locNo, int eventType, double rate) {
	return eventRates[iPop][eventType]->submitRate(locNo,rate);
}

double RateManager::submitRate_dumb(int iPop, int locNo, int eventType, double rate) {
	return eventRates[iPop][eventType]->submitRate_dumb(locNo,rate);
}

double RateManager::getRate(int iPop, int locNo, int eventType) {
	return eventRates[iPop][eventType]->getRate(locNo);
}

void RateManager::infRateScrub()
{
	for (int iPop = 0; iPop<PopulationCharacteristics::nPopulations; ++iPop) {
		for (int eventType = 0; eventType<PopulationCharacteristics::nPopulations; ++eventType) {
			eventRates[iPop][eventType]->scrubRates();//Infection from #Population = #eventType
		}
		eventRates[iPop][Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent]->scrubRates();
	}

	return;
}

void RateManager::infRateResum()
{
	for (int iPop = 0; iPop<PopulationCharacteristics::nPopulations; ++iPop) {
		for (int eventType = 0; eventType<PopulationCharacteristics::nPopulations; ++eventType) {
			eventRates[iPop][eventType]->fullResum();//Infection from #Population = #eventType
		}
		eventRates[iPop][Population::pGlobalPopStats[iPop]->iVirtualSporulationEvent]->fullResum();
	}

	return;
}

void RateManager::fullResum() {

	for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; ++iPop) {
		for(int eventType=0; eventType<Population::pGlobalPopStats[iPop]->nEvents; ++eventType) {
			eventRates[iPop][eventType]->fullResum();
		}
	}

	eventRatesExternal->fullResum();

	return;
}

int RateManager::requestExternalRate(Intervention *requestingIntervention, double dInitialRate, int iAdditionalArgument) {
	
	if(nEventsExternal<nMaxEventsExternal) {
		//Great, go ahead
	} else {
		//Ran out of room, need to reallocate:
		//TODO: Handle
		reportReadError("ERROR: Need to reallocate for external interventions\n");
	}

	externalRateInterventions[nEventsExternal] = requestingIntervention;
	piAdditionalArguments[nEventsExternal] = iAdditionalArgument;

	eventRatesExternal->submitRate(nEventsExternal, dInitialRate);

	return nEventsExternal++;//Do want to post increment
}

double RateManager::submitRateExternal(int iExternalEvent, double rate) {
	return eventRatesExternal->submitRate(iExternalEvent, rate);
}

//Event selection:
double RateManager::selectNextEvent() {

	double timeToNextEvent = 1.0e60;//Inordinately large value

	//apply Gillespie's (Direct) Algorithm
	double rTotal = getTotalRate();
	if(rTotal > 0.0) {
		//get time to next event
		timeToNextEvent = dExponentialVariate(rTotal);

		//Pick which event it is:
		double randRate = dUniformRandom()*rTotal;

		//Find if it was a Landscape or External event:
		double dLandscapeRatesTotal = getTotalRateLandscape();
		if(randRate < dLandscapeRatesTotal) {

			nextEventData.ES_Source = EVENTSOURCE_LANDSCAPE;

			//Find which population it occurred in:
			for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {
				double pRate = getTotalPopulationRate(iPop);
				if(pRate > randRate) {
					nextEventData.iLastEventPopType = iPop;
					break;
				} else {
					randRate -= pRate;
				}
			}

			//Find which type of event has occured:
			for(int iEvent=0; iEvent<Population::pGlobalPopStats[nextEventData.iLastEventPopType]->nEvents; iEvent++) {
				double eRate = getTotalEventRate(nextEventData.iLastEventPopType, iEvent);
				if(eRate > randRate) {
					nextEventData.iLastEventNo = iEvent;
					break;
				} else {
					randRate -= eRate;
				}
			}

			//Now search through super rate intervals for event #lastEventNo
			nextEventData.iLastLocation = getPopulationEventIndex(nextEventData.iLastEventPopType, nextEventData.iLastEventNo, randRate);

		} else {

			randRate -= dLandscapeRatesTotal;

			//External:
			nextEventData.ES_Source = EVENTSOURCE_EXTERNAL;

			//Find out which event it was:
			nextEventData.iExternalEvent = eventRatesExternal->getLocationEventIndex(randRate);

		}

	}

	return timeToNextEvent;
}

int RateManager::executeNextEvent() {

	switch(nextEventData.ES_Source) {
	case EVENTSOURCE_LANDSCAPE:
		if(nextEventData.iLastLocation >= 0 && nextEventData.iLastLocation < Population::pGlobalPopStats[nextEventData.iLastEventPopType]->nActiveCount) {
			//Actually have the event
			return world->EventLocations[nextEventData.iLastEventPopType][nextEventData.iLastLocation]->haveEvent(nextEventData.iLastEventPopType, nextEventData.iLastEventNo);
		} else {
			return 0;
		}
	case EVENTSOURCE_EXTERNAL:
		return externalRateInterventions[nextEventData.iExternalEvent]->execute(piAdditionalArguments[nextEventData.iExternalEvent]);
	default:
		return 0;
	}
}

double RateManager::getTotalRate() {
	
	double rTot = 0.0;

	rTot += getTotalRateLandscape();

	rTot += getTotalRateExternal();

	return rTot;
}

double RateManager::getTotalRateExternal() {
	
	double rTot = 0.0;

	//For each external event:
	for(int i=0; i<nEventsExternal; i++) {
		rTot += eventRatesExternal->getTotalRate();
	}

	return rTot;
}

double RateManager::getTotalRateLandscape() {
	
	double rTot = 0.0;

	//printf("DEBUG: Time: %12.6f\n", world->time);

	//For each Population:
	for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
		//For each event
		int nEvents = Population::pGlobalPopStats[i]->nEvents;
		for(int k=0; k<nEvents; k++) {
			double rEvent = pdRateConstantsEffective[i][k]*eventRates[i][k]->getTotalRate();
			rTot += rEvent;
			//printf("DEBUG: Event: %4d Rate: %12.8f\n", k, rEvent);
		}
	}

	//printf("DEBUG: TotalRate: %12.8f\n", rTot);

	return rTot;
}

double RateManager::getTotalPopulationRate(int iPop) {
	
	double rTot = 0.0;

	//For each event
	int nEvents = Population::pGlobalPopStats[iPop]->nEvents;
	for(int k=0; k<nEvents; k++) {
		rTot += getTotalEventRate(iPop, k);
	}

	return rTot;
}

double RateManager::getTotalEventRate(int iPop, int eventType) {
	return pdRateConstantsEffective[iPop][eventType]*eventRates[iPop][eventType]->getTotalRate();
}

//TODO: is this always used with correct assumptions viz Primary infection events?
int RateManager::scrubRates(int bCleanAllRates) {
	//Landscape being reset to clean status, remove all existing rate data:

	//For each Population:
	for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
		//For each event
		int nEvents = Population::pGlobalPopStats[i]->nEvents;
		for(int k=0; k<nEvents; k++) {

			if(bCleanAllRates) {
				eventRates[i][k]->scrubRates();
			}

			//Reset rate histories:
			rateConstantHistories[i][k].reset();

			//Reset rate scale factors to default, do this after resetting rate histories so histories log the first value
			scaleRateConstantFromDefault(i, k, 1.0);

		}
	}

	return 1;
}

int RateManager::getPopulationEventIndex(int iPop, int eventType, double rate) {

	//Scale out rate constant to find actual rate:
	double dActualRate = rate/pdRateConstantsEffective[iPop][eventType];

	return eventRates[iPop][eventType]->getLocationEventIndex(dActualRate);
}

double RateManager::getWorkToResubmit() {
	int nTotalWork = 0;
	int nTotalEvents = 0;

	//For each Population:
	for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
		//For each event
		int nEvents = Population::pGlobalPopStats[i]->nEvents;
		nTotalEvents += nEvents;
		for(int k=0; k<nEvents; k++) {
			nTotalWork += eventRates[i][k]->getWorkToResubmit();
		}
	}

	return double(nTotalWork)/double(nTotalEvents);
}

void RateManager::writeSummaryData(FILE *pSummaryFile) {

	//Dump out all the Rate constants:

	printHeaderText(pSummaryFile,"Rate Parameters:");

	fprintf(pSummaryFile, "%-24s %18.12f\n", "Rate_BackgroundInfection", pdRateConstantsDefault[0][Population::pGlobalPopStats[0]->iPrimaryInfectionEvent]);

	for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {

		char *szKeyName = new char[_N_MAX_DYNAMIC_BUFF_LEN];

		//Secondary infection Rate:
		sprintf(szKeyName,"Rate_%d_Sporulation",iPop);
		fprintf(pSummaryFile,"%-24s %18.12f\n", szKeyName, pdRateConstantsDefault[iPop][iPop]*PopulationCharacteristics::nMaxHosts);

		//Standard transition events:
		for(int iEvent = Population::pGlobalPopStats[iPop]->iStandardEventsStart; iEvent <= Population::pGlobalPopStats[iPop]->iStandardEventsEnd; ++iEvent) {
			getRateConstantNameFromPopAndEvent(iPop, iEvent, szKeyName);
			fprintf(pSummaryFile,"%-24s %18.12f\n", szKeyName, pdRateConstantsDefault[iPop][iEvent]);
		}

		fprintf(pSummaryFile, "\n");

		delete [] szKeyName;

	}

	fprintf(pSummaryFile,"\nSimulation rate structures: \n");
	fprintf(pSummaryFile,"RateStructureInfection: ");
	fprintf(pSummaryFile,"%s\n", RateStructureTypeNames[rsRateStructureToUseForRateSLoss]);
	fprintf(pSummaryFile,"RateStructureStandard: ");
	fprintf(pSummaryFile,"%s\n", RateStructureTypeNames[rsRateStructureToUseForStandards]);


	return;
}
