//ClimateSchemeScriptDouble.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "ClimateSchemeScriptDouble.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

ClimateSchemeScriptDouble::ClimateSchemeScriptDouble() {

}

ClimateSchemeScriptDouble::~ClimateSchemeScriptDouble() {

}

int ClimateSchemeScriptDouble::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szFileName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "VariableBackgroundInfection_Rates_%d", i_Scheme);
	sprintf(szFileName, "%s.txt", szRoot);

	//Initialisation:
	if(!world->bReadingErrorFlag && !world->bGiveKeys) {

		if(!readTimeStateValues(szFileName, nStates, pdTimes, pdValues)) {
			reportReadError("ERROR: %s unable to init from file %s\n", szRoot, szFileName);
		} else {
			//Nothing else to do, have array of doubles as desired
		}

		if(!world->bReadingErrorFlag) {
			reset();
		}

	}

	return 1;
}

double ClimateSchemeScriptDouble::getNextTime() {
	return timeNext;
}


int ClimateSchemeScriptDouble::applyNextTransition() {

	if (currentState + 1 < pdTimes.size()) {
		currentState++;
		timeNext = pdTimes[currentState];
	} else {
		//Ran out of states, so remain in last state until end of simulation:
		timeNext = world->timeEnd + 1.0e30;
	}

	return 1;
}

double ClimateSchemeScriptDouble::getCurrentValue() {
	return pdValues[currentState];
}

int ClimateSchemeScriptDouble::reset() {

	//If there are no times:
	currentState = -1;
	timeFirst = world->timeEnd + 1.0e30;
	timeNext = timeFirst;

	if (pdTimes.size() > 0) {
		//Some valid times:
		currentState = 0;

		while (currentState < pdTimes.size() && pdTimes[currentState] < world->timeStart) {
			currentState++;
		}

		if (currentState < pdTimes.size()) {
			timeFirst = pdTimes[currentState];
			timeNext = timeFirst;
		} else {
			//There are no times after the start of the simulation...
			currentState = -1;
			printk("Warning: VariableBackgroundInfection_Rates_%d specified %d times, but none of these times are after the simulation start time (%f). Last specified time %f.\n", iScheme, int(pdTimes.size()), world->timeStart, pdTimes[pdTimes.size() - 1]);
		}

	}

	return 1;
}
