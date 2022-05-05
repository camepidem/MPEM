//InoculumTrap.h

#pragma once

#include "classes.h"

class InoculumTrap {

public:

	InoculumTrap(int iPop=0, int iEvent=0, double dStartTime=0.0);
	~InoculumTrap();

	void reset();

	void virtualSpore(double dSporeQuantity);

	void reportRate(double dCurrentTime, double dCurrentUnscaledRate, RateManager *rateManager);
	
	void modifyRate(double dCurrentTime, double dChangeInUnscaledRate, RateManager *rateManager);

	double getCurrentAccumulation(double dCurrentTime, RateManager *rateManager);

protected:

	int iPopulation;
	int iEvent;

	double dStartTime;

	double dTimeLastRateChange;
	double dCurrentRate;

	double dAccumulatedInoculum;

};
