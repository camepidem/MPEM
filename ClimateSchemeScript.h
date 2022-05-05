//ClimateSchemeScript.h

#pragma once

#include "ClimateScheme.h"

class ClimateSchemeScript : public ClimateScheme {

public:

	ClimateSchemeScript();
	~ClimateSchemeScript();

	int init(int i_Scheme, Landscape *pWorld);

	int applyNextTransition();

	double getNextTime();

	int reset();

protected:

	int currentState;

	int nScripts;
	int iCurrentScript;

	vector<vector<double>> pdTimes;
	vector<vector<double>> pdStateFactors;

	int bRandomlySelectScripts;

	double timeFirst;
	double timeNext;

};
