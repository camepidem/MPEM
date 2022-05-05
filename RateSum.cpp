//RateSum.cpp

#include "stdafx.h"
#include "RateSum.h"

#pragma warning(disable : 4996)

RateSum::RateSum(int size) {

	nEvents = size;

	rates.resize(nEvents);

	//zero all the rates:
	scrubRates();

}

RateSum::~RateSum() {

}

double RateSum::submitRate(int locNo, double rate) {
	//Interface for a location to submit a rate for an event to the world event managing structure

	double dDelta = rate - rates[locNo];

	rates[locNo] = rate;

        // MC: 24-4-2014
	// dTotalRate += dDelta;
	ldTotalRate += dDelta;
	dTotalRate = (double) ldTotalRate;

	return rate;
}

double RateSum::getRate(int locNo) {
	return rates[locNo];
}

double RateSum::getTotalRate() {
	return dTotalRate;
}

int RateSum::scrubRates() {
	
	for(int iEvent = 0; iEvent < nEvents; iEvent++) {
		rates[iEvent] = 0.0;
	}

	dTotalRate = 0.0;

        // MC: 24-4-2014
	ldTotalRate = 0.0;
	return 1;
}

int RateSum::getLocationEventIndex(double rate) {

	int iSelectedEvent = 0;
	double dCumulativeRate = rates[0];

	while(dCumulativeRate < rate && iSelectedEvent < nEvents) {
		dCumulativeRate += rates[++iSelectedEvent];
	}

	return iSelectedEvent;
}

int RateSum::getWorkToResubmit() {
	return 1;
}

double RateSum::submitRate_dumb(int locNo, double rate) {
	return submitRate(locNo, rate);
}

int RateSum::fullResum() {

	dTotalRate = 0.0;

	for(int iEvent=0; iEvent<nEvents; iEvent++) {
		dTotalRate += rates[iEvent];
	}

	return 0;
}
