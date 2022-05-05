
#include "stdafx.h"
#include "cfgutils.h"
#include <limits.h>
#include "Landscape.h"
#include "ListManager.h"
#include "RateManager.h"
#include "Population.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "CohortTransitionSchemePeriodic.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

CohortTransitionSchemePeriodic::CohortTransitionSchemePeriodic()
{
}


CohortTransitionSchemePeriodic::~CohortTransitionSchemePeriodic()
{
}

int CohortTransitionSchemePeriodic::init(int i_Scheme, Landscape *pWorld) {

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

	timeFirst = world->timeStart;
	sprintf(szKey, "%s_First", szRoot);
	readValueToProperty(szKey, &timeFirst, 3, ">=0");

	period = 1.0;
	sprintf(szKey, "%s_Period", szRoot);
	readValueToProperty(szKey, &period, 3, ">0");
	if (period <= 0.0) {
		reportReadError("ERROR: %s must be > 0.0, read: %f\n", szKey, period);
	}

	//Initialisation:
	if (!world->bReadingErrorFlag && !world->bGiveKeys) {

		if (timeFirst < world->timeStart) {
			int nPeriodsBehind = ceil((world->timeStart - timeFirst) / period);
			double newTimeFirst = timeFirst + nPeriodsBehind * period;

			printk("Warning: %s specified first time %f, which is prior to simulation start time (%f).\nAdvancing first time to earliest time >= simulation start time by advancing start time %d periods to %f.\n", szKey, timeFirst, world->timeStart, nPeriodsBehind, newTimeFirst);

			timeFirst = newTimeFirst;
		}

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

		if (!world->bReadingErrorFlag) {
			reset();
		}

	}

	return 1;
}

double CohortTransitionSchemePeriodic::getNextTime() {
	return timeNext;
}


int CohortTransitionSchemePeriodic::applyNextTransition() {

	//Need to execute target events:
	for (int iControl = 0; iControl < nCoupledRates; iControl++) {

		LocationMultiHost *activeLocation;
		ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;

		//std::cerr << "CohortTransition controlling Population " << pRateConstantsControlledPop[iControl] << " Event " << pRateConstantsControlledEvent[iControl] << std::endl;

		int nChanged = 0;

		//Pass over all active locations
		while (pNode != NULL) {
			activeLocation = pNode->data;

			//Execute the event for all individuals capable of moving:
			nChanged += activeLocation->haveEvent(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], INT_MAX, true);

			pNode = pNode->pNext;
		}

		//std::cerr << "CohortTransition modified " << nChanged << " populations" << std::endl;

	}

	timeNext += period;

	return 1;
}

int CohortTransitionSchemePeriodic::reset() {

	timeNext = timeFirst;

	return 1;
}
