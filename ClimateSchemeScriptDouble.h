//ClimateSchemeScriptDouble.h

#pragma once

#include "ClimateScheme.h"

class ClimateSchemeScriptDouble : public ClimateScheme {

public:

	ClimateSchemeScriptDouble();
	~ClimateSchemeScriptDouble();

	int init(int i_Scheme, Landscape *pWorld);

	int applyNextTransition();

	double getNextTime();

	int reset();

	double getCurrentValue();

protected:

	int currentState;

	int nStates;
	vector<double> pdTimes;
	vector<double> pdValues;

	double timeFirst;
	double timeNext;

};
