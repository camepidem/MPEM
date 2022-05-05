//ClimateSchemeScript.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "Landscape.h"
#include "RateManager.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "ClimateSchemeScript.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

ClimateSchemeScript::ClimateSchemeScript() {

	nScripts = -1;

}

ClimateSchemeScript::~ClimateSchemeScript() {

}

int ClimateSchemeScript::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szKey[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "ClimateScheme_%d", iScheme);

	//Read which rate(s) to modulate:
	//TODO: For now just modify single rate:
	nCoupledRates = 1;
	char szRateConstant[_N_MAX_STATIC_BUFF_LEN];
	RateManager::getRateConstantNameFromPopAndEvent(0,0,szRateConstant);
	sprintf(szKey, "%s_RateConstant", szRoot);
	readValueToProperty(szKey, szRateConstant, 3, "{#RateConstants#}");

	nScripts = 1;
	sprintf(szKey, "%s_Scripts", szRoot);
	readValueToProperty(szKey,&nScripts,-3, ">0");

	bRandomlySelectScripts = 0;
	sprintf(szKey, "%s_RandomSelection", szRoot);
	readValueToProperty(szKey,&bRandomlySelectScripts,-3, "[0,1]");


	//Initialisation:
	if(!world->bReadingErrorFlag && !world->bGiveKeys) {

		int iTargetPop = -1;
		int iTargetEvent = -1;
		if(!RateManager::getPopAndEventFromRateConstantName(szRateConstant, iTargetPop, iTargetEvent)) {
			reportReadError("ERROR: %s unable to find rate constant %s\n", szRoot, szRateConstant);
		}

		//Check valid to modify:
		if(iTargetPop < 0 || iTargetPop > PopulationCharacteristics::nPopulations) {
			reportReadError("ERROR: In %s Target population %d is invalid\n", szRoot, iTargetPop);
		} else {

			if(iTargetEvent < 0 || iTargetEvent > Population::pGlobalPopStats[iTargetPop]->nEvents) {
				reportReadError("ERROR: In %s Target Event %d is invalid\n", szRoot, iTargetEvent);
			}

		}

		if(!world->bReadingErrorFlag) {

			pRateConstantsControlledPop.resize(nCoupledRates);
			pRateConstantsControlledEvent.resize(nCoupledRates);

			pRateConstantsControlledPop[0] = iTargetPop;
			pRateConstantsControlledEvent[0] = iTargetEvent;

		}


		char szFileName[FILENAME_MAX];

		pdTimes.resize(nScripts);
		pdStateFactors.resize(nScripts);

		for(iCurrentScript = 0; iCurrentScript<nScripts; iCurrentScript++) {

			sprintf(szFileName, "P_%d_ClimateData_%d.txt", iScheme, iCurrentScript);

			int nStates;
			if(!readIntFromCfg(szFileName, "States", &nStates)) {
				reportReadError("ERROR: Unable to find States header in climate file %s\n", szFileName);
			}

			if(!world->bReadingErrorFlag) {

				pdTimes[iCurrentScript].resize(nStates);
				pdStateFactors[iCurrentScript].resize(nStates);

				//Read climate file to arrays:
				if(!readTable(szFileName, nStates, 1, &pdTimes[iCurrentScript][0], &pdStateFactors[iCurrentScript][0])) {
					reportReadError("ERROR: Unable to read climate file: %s\n", szFileName);
				}

			}

		}

		if(!world->bReadingErrorFlag) {

			iCurrentScript = -1;//Just about to be incremented or randomly sampled in reset()
			
			reset();
		}


	}

	return 1;
}

double ClimateSchemeScript::getNextTime() {
	return timeNext;
}


int ClimateSchemeScript::applyNextTransition() {

	if (currentState >= 0) {
		//Need to update all rates in landscape that depended on its rate modifiers:
		for(int iControl=0; iControl<nCoupledRates; iControl++) {
			world->rates->scaleRateConstantFromDefault(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], pdStateFactors[iCurrentScript][currentState]);
		}

		if (currentState + 1 < pdTimes[iCurrentScript].size()) {
			currentState++;
			timeNext = pdTimes[iCurrentScript][currentState];
		} else {
			//Ran out of states, so remain in last state until end of simulation:
			currentState = -1;
			timeNext = world->timeEnd + 1.0e30;
		}
	} else {
		timeNext = world->timeEnd + 1.0e30;
	}

	return 1;
}

int ClimateSchemeScript::reset() {

	if(bRandomlySelectScripts) {
		iCurrentScript = int(dUniformRandom()*nScripts);
	} else {
		if(++iCurrentScript >= nScripts) {
			iCurrentScript = 0;//Loop if have run out of scripts:
		}
	}

	//Need to undo all rates in landscape that depended on its rate modifiers:
	for(int iControl=0; iControl<nCoupledRates; iControl++) {
		world->rates->scaleRateConstantFromDefault(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], 1.0);
	}

	//If there are no times:
	currentState = -1;
	timeFirst = world->timeEnd + 1.0e30;
	timeNext = timeFirst;

	if (pdTimes[iCurrentScript].size() > 0) {
		//Some valid times:
		currentState = 0;

		while (currentState < pdTimes[iCurrentScript].size() && pdTimes[iCurrentScript][currentState] < world->timeStart) {
			currentState++;
		}

		if (currentState < pdTimes[iCurrentScript].size()) {
			timeFirst = pdTimes[iCurrentScript][currentState];
			timeNext = timeFirst;
		} else {
			//There are no times after the start of the simulation...
			currentState = -1;
			printk("Warning: ClimateScheme_%d specified %d times, but none of these times are after the simulation start time (%f). Last specified time %f.\n", iScheme, int(pdTimes[iCurrentScript].size()), world->timeStart, pdTimes[iCurrentScript][pdTimes[iCurrentScript].size() - 1]);
		}

	}

	return 1;
}
