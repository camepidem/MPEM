//CohortTransitionSchemeScript.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include <limits.h>
#include "myRand.h"
#include "Landscape.h"
#include "ListManager.h"
#include "RateManager.h"
#include "Population.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "CohortTransitionSchemeScript.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

CohortTransitionSchemeScript::CohortTransitionSchemeScript() {

	nScripts = -1;

}

CohortTransitionSchemeScript::~CohortTransitionSchemeScript() {

}

int CohortTransitionSchemeScript::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szKey[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "CohortTransitionScheme_%d", iScheme);

	//Read which rate(s) to modulate:
	//TODO: For now just modify single rate:
	nCoupledRates = 1;
	char szRateConstant[_N_MAX_STATIC_BUFF_LEN];
	RateManager::getRateConstantNameFromPopAndEvent(0, 0, szRateConstant);
	sprintf(szKey, "%s_RateConstant", szRoot);
	readValueToProperty(szKey, szRateConstant, 3, "{#RateConstants#}");

	nScripts = 1;
	sprintf(szKey, "%s_Scripts", szRoot);
	readValueToProperty(szKey, &nScripts, -3, ">0");

	bRandomlySelectScripts = 0;
	sprintf(szKey, "%s_RandomSelection", szRoot);
	readValueToProperty(szKey, &bRandomlySelectScripts, -3, "[0,1]");


	//Initialisation:
	if (!world->bReadingErrorFlag && !world->bGiveKeys) {

		int iTargetPop = -1;
		int iTargetEvent = -1;
		if (!RateManager::getPopAndEventFromRateConstantName(szRateConstant, iTargetPop, iTargetEvent)) {
			reportReadError("ERROR: %s unable to find rate constant %s\n", szRoot, szRateConstant);
		}

		//Check valid to modify:
		if (iTargetPop < 0 || iTargetPop > PopulationCharacteristics::nPopulations) {
			reportReadError("ERROR: In %s Target population %d is invalid\n", szRoot, iTargetPop);
		} else {

			if (iTargetEvent < 0 || iTargetEvent > Population::pGlobalPopStats[iTargetPop]->nEvents) {
				reportReadError("ERROR: In %s Target Event %d is invalid\n", szRoot, iTargetEvent);
			}

		}

		if (!world->bReadingErrorFlag) {

			pRateConstantsControlledPop.resize(nCoupledRates);
			pRateConstantsControlledEvent.resize(nCoupledRates);

			pRateConstantsControlledPop[0] = iTargetPop;
			pRateConstantsControlledEvent[0] = iTargetEvent;

		}


		char szFileName[FILENAME_MAX];

		pdTimes.resize(nScripts);

		for (iCurrentScript = 0; iCurrentScript<nScripts; iCurrentScript++) {

			sprintf(szFileName, "P_%d_CohortTransitionData_%d.txt", iScheme, iCurrentScript);

			int nStates;
			if (!readIntFromCfg(szFileName, "States", &nStates)) {
				reportReadError("ERROR: Unable to find States header in CohortTransition file %s\n", szFileName);
			}

			if (!world->bReadingErrorFlag) {

				pdTimes[iCurrentScript].resize(nStates);

				//Read cohort file to arrays:
				if (!readTable(szFileName, nStates, 1, &pdTimes[iCurrentScript][0])) {
					reportReadError("ERROR: Unable to read CohortTransition file: %s\n", szFileName);
				}

			}

		}

		if (!world->bReadingErrorFlag) {

			iCurrentScript = 0;
			if (bRandomlySelectScripts) {
				iCurrentScript = int(dUniformRandom()*nScripts);
			}

			reset();

			if (currentState > 0) {
				printk("Warning: CohortTransitionScheme_%d specified %d times, but %d are before simulation start time (%f). Times prior to %f are being ignored.\n", iScheme, int(pdTimes[iCurrentScript].size()), world->timeStart, timeFirst);
			}
		}


	}

	return 1;
}

double CohortTransitionSchemeScript::getNextTime() {
	return timeNext;
}


int CohortTransitionSchemeScript::applyNextTransition() {

	if (currentState >= 0) {
		
		//Need to execute target events:
		for (int iControl = 0; iControl<nCoupledRates; iControl++) {

			LocationMultiHost *activeLocation;
			ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;

			//Pass over all active locations
			while (pNode != NULL) {
				activeLocation = pNode->data;
				
				//Execute the event for all individuals capable of moving:
				activeLocation->haveEvent(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], INT_MAX, true);

				pNode = pNode->pNext;
			}

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

int CohortTransitionSchemeScript::reset() {

	if (bRandomlySelectScripts) {
		iCurrentScript = int(dUniformRandom()*nScripts);
	} else {
		if (++iCurrentScript >= nScripts) {
			iCurrentScript = 0;//Loop if have run out of scripts:
		}
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
			printk("Warning: CohortTransitionScheme_%d specified %d times, but none of these times are after the simulation start time (%f). Last specified time %f.\n", iScheme, int(pdTimes[iCurrentScript].size()), world->timeStart, pdTimes[iCurrentScript][pdTimes[iCurrentScript].size() - 1]);
		}

	}

	return 1;
}
