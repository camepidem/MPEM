
#pragma once

class CohortTransitionScheme {

public:

	CohortTransitionScheme();
	virtual ~CohortTransitionScheme();

	virtual int init(int i_Scheme, Landscape *pWorld);

	virtual int applyNextTransition();

	virtual double getNextTime();

	virtual int reset();

protected:

	Landscape *world;

	int iScheme;

	int nCoupledRates;
	vector<int> pRateConstantsControlledPop;
	vector<int> pRateConstantsControlledEvent;
};

