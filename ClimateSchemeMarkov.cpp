//ClimateSchemeMarkov.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "Landscape.h"
#include "RateManager.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "ClimateSchemeMarkov.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

//TODO: Generalise to N states and allow specifying starting state:
ClimateSchemeMarkov::ClimateSchemeMarkov() {

}

ClimateSchemeMarkov::~ClimateSchemeMarkov() {

}

int ClimateSchemeMarkov::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szKey[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "ClimateScheme_%d", iScheme);

	//Read which rate(s) to modulate:
	//For now just modify single rate:
	nCoupledRates = 1;
	char szRateConstant[_N_MAX_STATIC_BUFF_LEN];
	RateManager::getRateConstantNameFromPopAndEvent(0,0,szRateConstant);
	sprintf(szKey, "%s_RateConstant", szRoot);
	readValueToProperty(szKey, szRateConstant, 3, "{#RateConstants#}");

	sprintf(szKey, "%s_Frequency", szRoot);
	frequency = 1.0;
	readValueToProperty(szKey,&frequency,3, ">0");

	if (frequency <= 0.0) {
		reportReadError("ERROR: %s must be > 0.0, read: %f", szKey, frequency);
	}

	sprintf(szKey, "%s_First", szRoot);
	timeFirst = world->timeStart;
	readValueToProperty(szKey,&timeFirst,3, ">0");

	double transProb00 = 0.0, transProb11 = 0.0, stateFactor1 = 1.0;
	sprintf(szKey, "%s_Stay0", szRoot);
	readValueToProperty(szKey,&transProb00,3, "[0,1]");

	sprintf(szKey, "%s_Stay1", szRoot);
	readValueToProperty(szKey,&transProb11,3, "[0,1]");

	sprintf(szKey, "%s_Factor", szRoot);
	readValueToProperty(szKey,&stateFactor1,3, "[0,1]");


	//Initialisation:
	if(!world->bReadingErrorFlag && !world->bGiveKeys) {

		if (timeFirst < world->timeStart - frequency) {
			int nPeriodsBehind = ceil((world->timeStart - timeFirst) / frequency);
			double newTimeFirst = timeFirst + (nPeriodsBehind - 1) * frequency;

			printk("Warning: %s specified first time %f, which is more than one period prior to simulation start time (%f).\nAdvancing first time to latest time < simulation start time by advancing start time %d periods to %f.\n", szRoot, timeFirst, world->timeStart, (nPeriodsBehind - 1), newTimeFirst);

			timeFirst = newTimeFirst;
		}

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

			nStates = 2;

			stateFactors.resize(nStates);

			stateFactors[0] = 1.0;
			stateFactors[1] = stateFactor1;

			stateTransitionProbs.resize(nStates);
			for(int i = 0; i<nStates; i++) {
				stateTransitionProbs[i].resize(nStates);
			}


			stateTransitionProbs[0][0] = transProb00;
			stateTransitionProbs[0][1] = 1.0-transProb00;
			stateTransitionProbs[1][0] = 1.0-transProb11;
			stateTransitionProbs[1][1] = transProb11;

			reset();

		}

	}

	return 1;
}

double ClimateSchemeMarkov::getNextTime() {
	return timeNext;
}


int ClimateSchemeMarkov::applyNextTransition() {

	//Swap states:
	double randomNo = dUniformRandom();

	double totalProb = 0.0;

	for(int i = 0; i<nStates; i++) {
		totalProb += stateTransitionProbs[currentState][i];
		if(totalProb>=randomNo) {
			currentState = i;
			break;
		}
	}

	//Need to update all rates in landscape that depended on its rate modifiers:
	for(int iControl=0; iControl<nCoupledRates; iControl++) {
		world->rates->scaleRateConstantFromDefault(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], stateFactors[currentState]);
	}


	timeNext += frequency;

	return 1;
}

int ClimateSchemeMarkov::reset() {

	currentState = 0;

	timeNext = timeFirst;

	//Need to undo all rates in landscape that depended on its rate modifiers:
	for(int iControl=0; iControl<nCoupledRates; iControl++) {
		world->rates->scaleRateConstantFromDefault(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], 1.0);
	}

	return 1;
}
