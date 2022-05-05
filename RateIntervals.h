//RateIntervals.h

#ifndef RATEINTERVALS_H
#define RATEINTERVALS_H 1

#include "RateStructure.h"

class RateIntervals : public RateStructure {

public:

	//Constructor/destructor:
	RateIntervals(int size);
	~RateIntervals();

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

	//Input to structure automatically summing, useful for resubmitting large portion of rate structure, must be followed by a fullResum() before structure is used
	double submitRate_dumb(int locNo, double rate);

	//Forces recalculation of whole rate structure:
	int fullResum();

protected:

	//Data:

	//Length of interval
	int nIntervalLength;

	//Length of event arrays, padded to multiple of nIntervalLength
	int nPaddedLength;
	
	//Number of sub intervals:
	int nIntervals;

	//Array of individual rates:
	vector<double> subIndividualRates;
	//Cumulative sums of individual rates
	vector<double> subSumRates;

	//Individual Sums of interval rates
	vector<double> superIndividualRates;
	//Cumulative sums of interval rates
	vector<double> superSumRates;

	//Flag for if interval's cumulative sum is correct
	//The interval's sum is never correct, this is superfluous
	//int *intervalCumulativeSumCorrect;

	//Flag if any rate change has occured: value of flag is lowest index of interval to have a rate change:
	int nGlobalFlag;


	//Total event rate:
	double totalRate;


	//Methods:

	//Returns index of subinterval containing marginal rate
	int intervalSearchSuper(double rate);

	//Returns index of event with marginal rate from within subinterval cumulative sum
	int intervalSearchSub(double rate, int nInterval);


	int sumSuperRates();

	double sumIntervalRate(int nIntervalToSum);



};

#endif
