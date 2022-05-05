//RateSum.h

#pragma once

#include "RateStructure.h"

class RateSum : public RateStructure {

public:

	//Constructor/destructor:
	RateSum(int size);
	~RateSum();

	//Interface methods:
	
	//Rate reporting:
	//Location submits rate:
	double submitRate(int locNo, double rate);
	//Retrieve rate for given location:
	double getRate(int locNo);

	//Find total rate of event:
	double getTotalRate();

	//Find location for given marginal rate 
	int getLocationEventIndex(double rate);

	//cleans out all rates to 0.0 (part of interventions)
	int scrubRates();

	//Find approx number of operations to resubmit all rates (used when landscape needs to be cleared; may be easier to just manually set all rates to 0.0)
	int getWorkToResubmit();

	//Input to array without automatically summing, useful for resubmitting large portion of tree, must be followed by a fullResum() before tree is used
	double submitRate_dumb(int locNo, double rate);

	//Forces recalculation of whole tree from bottom up
	int fullResum();

protected:

	int nEvents;
	vector<double> rates;//Holds all the rates, index is that of location

	//TODO: is there any point in having this, as on selection an event occurs making it obsolete again anyway...
	//double *pdCumulativeRates;//

	double dTotalRate;
        // MC: 24-4-2014
	long double ldTotalRate;

};
