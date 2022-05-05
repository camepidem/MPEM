//RateConstantHistory.cpp

#include "stdafx.h"
#include <algorithm>
#include "RateConstantHistory.h"

RateConstantHistory::RateConstantHistory() {
	
	reset();

}

RateConstantHistory::~RateConstantHistory() {

}

void RateConstantHistory::changeRateConstant(double dTimeNow, double dNewValue) {

	times.push_back(dTimeNow);
	rates.push_back(dNewValue);

	return;
}

double RateConstantHistory::getAverage(double dTimeStart, double dTimeEnd) {

	double dTimeIntervalLength = dTimeEnd - dTimeStart;

	double dTotalRate = 0.0;

	//Caching:
	/*if (lastRequestCache.bMatchesRequest(dTimeStart, dTimeEnd)) {
		return lastRequestCache.getCachedAverage();
	}*/

	if(times.size() > 0) {

		//Assume rate constant is zero before the beginning of times and stays at last value seen forever
		if(dTimeStart < times[0]) {
			dTimeStart = times[0];
		}

		//Upper bound gives the first element that is greater than time
		//if no time element is found then it returns last, which when decrenented will give the final time/rate pair *as desired*
		auto startRateIt = std::upper_bound(times.begin(), times.end(), dTimeStart);
		if(startRateIt != times.begin()) {
			//Previous element (if it exists) will be the last element less than time
			//If we got the first element, then we don't need to calculate before then as the rate is zero before the first time anyway
			--startRateIt;
		}
		auto endRateIt = std::upper_bound(times.begin(), times.end(), dTimeEnd);
		if(endRateIt != times.begin()) {
			//Previous element (if it exists) will be the last element less than time
			--endRateIt;
		}

		size_t startIndex = startRateIt - times.begin();
		size_t endIndex = endRateIt - times.begin();

		size_t currentIndex = startIndex;
		double currentTime = max(dTimeStart, times[currentIndex]);//Rates assumed zero before recording begins, so take no time before then

		while(currentIndex < endIndex) {

			double currentRate = rates[currentIndex];
			double nextTime = times[currentIndex + 1];

			double dTimeInInterval = nextTime - currentTime;

			dTotalRate += dTimeInInterval*currentRate;

			currentTime = times[++currentIndex];

		}

		//Do last block which is a bit special
		double dTimeInInterval = dTimeEnd - currentTime;
		dTotalRate += dTimeInInterval*rates[endIndex];

		double dAverageRate = dTotalRate;
		if(dTimeIntervalLength > 0.0) {
			dAverageRate /= dTimeIntervalLength;
		} else {
			//Average over a point estimate should just be the value at that point
			dAverageRate = rates[endIndex];
		}

		//Caching:
		//Cache the result:
		//lastRequestAverage = RateConstantHistoryAverage(dTimeStart, dTimeEnd, dAverageRate);

		return dAverageRate;

	}

	//If we haven't had any rates yet, assume zero
	return 0.0;

}

void RateConstantHistory::reset() {

	times.resize(0);
	rates.resize(0);

	return;
}

RateConstantHistoryAverage::RateConstantHistoryAverage() :
	dTimeStart_(-1.0), dTimeEnd_(-1.0), dAverageValue_(-1.0)
{
}

RateConstantHistoryAverage::RateConstantHistoryAverage(double dTimeStart, double dTimeEnd, double dAverageValue) :
	dTimeStart_(dTimeStart), dTimeEnd_(dTimeEnd), dAverageValue_(dAverageValue)
{

}

RateConstantHistoryAverage::~RateConstantHistoryAverage()
{
}

bool RateConstantHistoryAverage::bMatchesRequest(double dTimeStart, double dTimeEnd)
{

	if (dTimeStart == dTimeStart_ && dTimeEnd == dTimeEnd_) {
		return true;
	}

	return false;
}

double RateConstantHistoryAverage::getCachedAverage()
{
	return dAverageValue_;
}
