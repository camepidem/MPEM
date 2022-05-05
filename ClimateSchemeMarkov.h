//ClimateSchemeMarkov.h

#pragma once

#include "ClimateScheme.h"

class ClimateSchemeMarkov : public ClimateScheme {

public:

	ClimateSchemeMarkov();
	~ClimateSchemeMarkov();

	int init(int i_Scheme, Landscape *pWorld);

	int applyNextTransition();

	double getNextTime();

	int reset();

protected:

	double frequency;
	double timeFirst;
	double timeNext;

	int nStates;

	vector<double> stateFactors;

	vector<vector<double>> stateTransitionProbs;

	int currentState;

};
