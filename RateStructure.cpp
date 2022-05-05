//RateStructure.cpp

#include "stdafx.h"
#include "RateStructure.h"

#pragma warning(disable : 4996)

RateStructure::RateStructure() {

}

RateStructure::~RateStructure() {

}

double RateStructure::submitRate(int locNo, double rate) {
	return -1.0;
}

double RateStructure::submitRate_dumb(int locNo, double rate) {
	return -1.0;
}

int RateStructure::fullResum() {
	return -1;
}

double RateStructure::getRate(int locNo) {
	return -1.0;
}

double RateStructure::getTotalRate() {
	return -1.0;
}

int RateStructure::scrubRates() {
	return -1;
}

int RateStructure::getLocationEventIndex(double rate) {
	return -1;
}

int RateStructure::getWorkToResubmit() {
	return -1;
}
