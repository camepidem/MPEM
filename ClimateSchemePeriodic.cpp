
#include "stdafx.h"
#include "cfgutils.h"
#include <limits.h>
#include "Landscape.h"
#include "ListManager.h"
#include "RateManager.h"
#include "Population.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "ClimateSchemePeriodic.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

ClimateSchemePeriodic::ClimateSchemePeriodic()
{
}


ClimateSchemePeriodic::~ClimateSchemePeriodic()
{
}

int ClimateSchemePeriodic::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szKey[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "ClimateScheme_%d", iScheme);

	//Read which rate(s) to modulate:
	//TODO: For now just modify single rate:
	nCoupledRates = 1;
	char szRateConstant[_N_MAX_STATIC_BUFF_LEN];
	RateManager::getRateConstantNameFromPopAndEvent(0, 0, szRateConstant);
	sprintf(szKey, "%s_RateConstant", szRoot);
	readValueToProperty(szKey, szRateConstant, 3, "{#RateConstants#}");

	timeFirst = world->timeStart;
	sprintf(szKey, "%s_First", szRoot);
	readValueToProperty(szKey, &timeFirst, 3, "");

	period = 1.0;
	sprintf(szKey, "%s_Period", szRoot);
	readValueToProperty(szKey, &period, 3, ">0");

	if (period <= 0.0) {
		reportReadError("ERROR: %s must be > 0.0, read: %f", szKey, period);
	}

	duration = 1.0;
	sprintf(szKey, "%s_OnDuration", szRoot);
	readValueToProperty(szKey, &duration, 3, ">0");
	if (duration > period) {
		reportReadError("ERROR: %s OnDuration (%f) specified as longer than period(%f)\n", szRoot, duration, period);
	}

	offFactor = 1.0;
	sprintf(szKey, "%s_OffFactor", szRoot);
	readValueToProperty(szKey, &offFactor, -3, ">0");
	if (offFactor < 0.0) {
		reportReadError("ERROR: %s OffFactor (%f) less than zero\n", szRoot, offFactor);
	}

	onFactor = 1.0;
	sprintf(szKey, "%s_OnFactor", szRoot);
	readValueToProperty(szKey, &onFactor, -3, ">0");
	if (onFactor < 0.0) {
		reportReadError("ERROR: %s OnFactor (%f) less than zero\n", szRoot, onFactor);
	}


	//Initialisation:
	if (!world->bReadingErrorFlag && !world->bGiveKeys) {

		if (timeFirst < world->timeStart - period) {
			int nPeriodsBehind = floor((world->timeStart - timeFirst) / period);
			double newTimeFirst = timeFirst + nPeriodsBehind * period;

			printk("Warning: %s specified first time %f, which is more than one period prior to simulation start time (%f).\nAdvancing first time to latest time < simulation start time by advancing start time %d periods to %f.\n", szRoot, timeFirst, world->timeStart, nPeriodsBehind, newTimeFirst);

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

double ClimateSchemePeriodic::getNextTime() {
	return timeNext;
}


int ClimateSchemePeriodic::applyNextTransition() {

	if (currentState == -1) {
		//Special case to make sure we're in the off state at the very start, until the first change.
		timeNext = timeFirst;
		currentState = 0;
	} else {
		if (currentState == 1) {
			timeNext += period - duration;
			currentState = 0;
		} else {
			timeNext += duration;
			currentState = 1;
		}
	}

	double stateFactor = offFactor;

	if (currentState == 1) {
		stateFactor = onFactor;
	}

	//Need to update all rates in landscape that depended on its rate modifiers:
	for (int iControl = 0; iControl<nCoupledRates; iControl++) {
		world->rates->scaleRateConstantFromDefault(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], stateFactor);
	}

	return 1;
}

int ClimateSchemePeriodic::reset() {

	//Need to undo all rates in landscape that depended on its rate modifiers:
	for (int iControl = 0; iControl<nCoupledRates; iControl++) {
		world->rates->scaleRateConstantFromDefault(pRateConstantsControlledPop[iControl], pRateConstantsControlledEvent[iControl], 1.0);
	}

	currentState = -1;

	timeNext = 0.0;

	return 1;
}
