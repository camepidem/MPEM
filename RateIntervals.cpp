//RateIntervals.cpp

#include "stdafx.h"
#include "RateIntervals.h"

#pragma warning(disable : 4996)

RateIntervals::RateIntervals(int size) {

	//TODO: Assess optimal length for intervals:
	nIntervalLength = int(sqrt(double(size)));

	//Ensure length is at least 1:
	if(nIntervalLength<1) {
		nIntervalLength = 1;
	}

	nIntervals = int(ceil(double(size)/double(nIntervalLength)));

	nPaddedLength = nIntervals*nIntervalLength;

	//Storage for rates:
	subIndividualRates.resize(nPaddedLength);

	//Storage for interval sumulative sums:
	subSumRates.resize(nPaddedLength);

	//Cumulative Interval structure:

	//Storage for interval totals:
	superIndividualRates.resize(nIntervals);

	//Storage for cumulative sums: 
	superSumRates.resize(nIntervals);

	//zero all the rates:
	scrubRates();

}

RateIntervals::~RateIntervals() {

}

double RateIntervals::submitRate(int locNo, double rate) {

	double delta = rate - subIndividualRates[locNo];

	//Store new rate:
	subIndividualRates[locNo] = rate;

	//Report change to interval structure:
	int intervalNo = locNo/nIntervalLength;

	//Flag that change has taken place in interval:
	//Flag that cumulative sum within interval is now incorrect:
	//intervalCumulativeSumCorrect[intervalNo] = 0;

	//Flag that (at least) this interval has changed:
	if(intervalNo < nGlobalFlag) {
		nGlobalFlag = intervalNo;
	}

	//Update total rate of this interval:
	superIndividualRates[intervalNo] += delta;

	//Update total rate:
	totalRate += delta;


	return rate;
}

double RateIntervals::getRate(int locNo) {

	return subIndividualRates[locNo];

}

double RateIntervals::getTotalRate() {

	if(nGlobalFlag < nIntervals) {
		//There has been a change to an interval:
		sumSuperRates();

		totalRate = superSumRates[nIntervals-1];

		return totalRate;

	} else {
		//There have been no changes, return stored value for total rate:
		return totalRate;
	}

}

int RateIntervals::scrubRates() {

	//Zero all sub rate data:
	for(int i=0; i<nPaddedLength; i++) {
		subIndividualRates[i] = 0.0;
		subSumRates[i] = 0.0;
	}

	//zero all super rate data:
	for(int i=0; i<nIntervals; i++) {
		superIndividualRates[i] = 0.0;
		superSumRates[i] = 0.0;
		//mark cumulative sum as correct
		//intervalCumulativeSumCorrect[i] = 1;
	}

	//Total rate information:
	nGlobalFlag = nIntervals;

	totalRate = 0.0;


	return 1;
}

int RateIntervals::getLocationEventIndex(double rate) {

	int nIntervalMarginal = intervalSearchSuper(rate);

	double dRateMarginal = rate;
	if(nIntervalMarginal != 0) {
		dRateMarginal = rate - superSumRates[nIntervalMarginal - 1];
	}

	return intervalSearchSub(dRateMarginal,nIntervalMarginal);

}

int RateIntervals::getWorkToResubmit() {

	//TODO:

	return 1;
}

double RateIntervals::submitRate_dumb(int locNo, double rate) {

	return submitRate(locNo, rate);

}

int RateIntervals::fullResum() {

	//Unneccessary for Intervals

	return 0;
}

int RateIntervals::sumSuperRates() {

	if(nGlobalFlag == 0) {
		superSumRates[0] = superIndividualRates[0];
		nGlobalFlag = 1;
	}

	double val;
	double *first = &superIndividualRates[0];
	double *last = &superIndividualRates[nIntervals];
	double *result = &superSumRates[0];
	*result++ = val = *first++;
	while(first!=last) {
		*result++ = val = val + *first++;
	}


	//Flag that have summed up to the end:
	nGlobalFlag = nIntervals;

	return 1;

}

double RateIntervals::sumIntervalRate(int nIntervalToSum) {

	int nFirstElement = nIntervalToSum*nIntervalLength;
	int nLastElement = nFirstElement + nIntervalLength - 1;

	//Get cumulative sum:

	//want the sum to start from 0, so do first term outside loop:
	double val;
	double *first = &subIndividualRates[nFirstElement];
	double *last = &subIndividualRates[nLastElement+1];
	double *result = &subSumRates[nFirstElement];
	*result++ = val = *first++;
	while(first!=last) {
		*result++ = val = val + *first++;
	}


	//Flag interval now has correct cumulative sum: pointless
	//intervalCumulativeSumCorrect[nIntervalToSum] = 1;

	//Update intervals superRate (eliminates some floating point errors)
	superIndividualRates[nIntervalToSum] = subSumRates[nLastElement];

	//Return total sum of interval:
	return subSumRates[nLastElement];
}

int RateIntervals::intervalSearchSuper(double rate) {

	//finds smallest i such that superSumRates[i]>rate by interval bisection
	int nLow, nMid, nHigh;

	nLow = 0;
	nHigh = nIntervals-1;
	while(nHigh - nLow > 1) {
		nMid = nLow + (nHigh - nLow)/2;
		if(superSumRates[nMid] > rate) {
			nHigh = nMid;
		} else {
			nLow = nMid;
		}
	}
	if(superSumRates[nLow] > rate) {
		return nLow;
	} else {
		return nHigh;
	}

}

int RateIntervals::intervalSearchSub(double rate, int nInterval) {

	//Ensure that cumulative sum of interval is correct:
	sumIntervalRate(nInterval);

	//No point using this, as we are just about to have an event here which will obviously make the cumulative sum wrong again...
	/*if(!intervalCumulativeSumCorrect[nInterval]) {
	sumIntervalRate(nInterval);
	}*/

	//finds smallest i such that subSumRates[i]>rate by interval bisection
	int nLow, nMid, nHigh;

	nLow = nInterval*nIntervalLength;
	nHigh = nLow + nIntervalLength - 1;
	while(nHigh - nLow > 1) {
		nMid = nLow + (nHigh - nLow)/2;
		if(subSumRates[nMid] > rate) {
			nHigh = nMid;
		} else {
			nLow = nMid;
		}
	}
	if(subSumRates[nLow] > rate) {
		return nLow;
	} else {
		return nHigh;
	}

}
