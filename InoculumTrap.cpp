//InoculumTrap.cpp

#include "stdafx.h"
#include "InoculumTrap.h"
#include "RateManager.h"

InoculumTrap::InoculumTrap(int iPop, int iEv, double dTimeStart){

	iPopulation = iPop;
	iEvent = iEv;

	dStartTime = dTimeStart;

	reset();

}

InoculumTrap::~InoculumTrap(){

}

void InoculumTrap::reset(){

	dTimeLastRateChange = dStartTime;
	dCurrentRate = 0.0;

	dAccumulatedInoculum = 0.0;

}

void InoculumTrap::virtualSpore(double dSporeQuantity) {

	//Should not need RateManager as rate constant used already to determine sporulation rates
	dAccumulatedInoculum += dSporeQuantity;

}

void InoculumTrap::reportRate(double dCurrentTime, double dCurrentUnscaledRate, RateManager *rateManager){

	double deltaT = dCurrentTime - dTimeLastRateChange;

	double dAverageRateConstant = rateManager->getRateConstantAverage(iPopulation, iEvent, dTimeLastRateChange, dCurrentTime);

	dAccumulatedInoculum += deltaT*dCurrentRate*dAverageRateConstant;

	dCurrentRate = dCurrentUnscaledRate;
	dTimeLastRateChange = dCurrentTime;

}

void InoculumTrap::modifyRate(double dCurrentTime, double dChangeInUnscaledRate, RateManager *rateManager){

	reportRate(dCurrentTime, dCurrentRate + dChangeInUnscaledRate, rateManager);

}

double InoculumTrap::getCurrentAccumulation(double dCurrentTime, RateManager *rateManager){

	if(dCurrentTime != dTimeLastRateChange) {
		//Really hope current time is not less than last time...
		reportRate(dCurrentTime, dCurrentRate, rateManager);
	}

	return dAccumulatedInoculum;

}
