#pragma once

#include "CohortTransitionScheme.h"

class CohortTransitionSchemeScript : public CohortTransitionScheme
{
public:

	CohortTransitionSchemeScript();
	~CohortTransitionSchemeScript();

	int init(int i_Scheme, Landscape *pWorld);

	int applyNextTransition();

	double getNextTime();

	int reset();

protected:

	int currentState;

	int nScripts;
	int iCurrentScript;

	vector<vector<double>> pdTimes;

	int bRandomlySelectScripts;

	double timeFirst;
	double timeNext;

};

