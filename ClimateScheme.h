//ClimateScheme.h

#pragma once

class ClimateScheme {

public:

	ClimateScheme();
	virtual ~ClimateScheme();

	virtual int init(int i_Scheme, Landscape *pWorld);

	virtual int applyNextTransition();

	virtual double getNextTime();

	virtual int reset();

	static ClimateScheme *newScript(char scriptID);

protected:

	Landscape *world;

	int iScheme;

	int nCoupledRates;
	vector<int> pRateConstantsControlledPop;
	vector<int> pRateConstantsControlledEvent;

};
