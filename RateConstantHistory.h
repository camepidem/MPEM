//RateConstantHistory.h

#pragma once


//Caching:
class RateConstantHistoryAverage {

public:

	RateConstantHistoryAverage();
	RateConstantHistoryAverage(double dTimeStart, double dTimeEnd, double dAverageValue);
	~RateConstantHistoryAverage();

	bool bMatchesRequest(double dTimeStart, double dTimeEnd);

	double getCachedAverage();

protected:

	double dTimeStart_;
	double dTimeEnd_;
	double dAverageValue_;

};


class RateConstantHistory {

public:

	RateConstantHistory();
	~RateConstantHistory();

	void changeRateConstant(double dTimeNow, double dNewValue);

	double getAverage(double dTimeStart, double dTimeEnd);

	void reset();

protected:

	vector<double> times;
	vector<double> rates;

	//Caching
	RateConstantHistoryAverage lastRequestAverage;

};
