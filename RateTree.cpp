//RateTree.cpp

#include "stdafx.h"
#include "RateTree.h"

#pragma warning(disable : 4996)

RateTree::RateTree(int size) {

	//Create tree structure for event:

	//Pad arrays to length 2^n for tree
	nTreeLevels = 1;
	nPaddedLength = 1;
	while(nPaddedLength < size) {
		nPaddedLength *= 2;
		nTreeLevels++;
	}

	rates.resize(2*nPaddedLength - 1);

	treeLevels.resize(nTreeLevels + 2);

	//Pad arrays with pointers to null on either end:
	treeLevels[0] = NULL;
	treeLevels[nTreeLevels+1] = NULL;
	int nLevelLength = nPaddedLength;
	int nLevelStart = 0;
	for(int j=1; j<=nTreeLevels; j++) {
		treeLevels[j] = &rates[nLevelStart];
		nLevelStart += nLevelLength;
		nLevelLength /= 2;
	}

	//zero all the rates:
	scrubRates();

}

RateTree::~RateTree() {

}

double RateTree::submitRate(int locNo, double rate) {
	//Interface for a location to submit a rate for an event to the world event managing structure
	//Handles storage of rate and manages tree structure

	double **myPtr = &treeLevels[1];

	// double dDelta = rate - *(*myPtr+locNo);
	long double dDelta = (long double) rate - (long double) *(*myPtr+locNo);

        // MC: 26-4-2014
        ldTotalRate += dDelta;

	while(*myPtr != NULL) {
		// *(*myPtr + locNo) += dDelta;
		*(*myPtr + locNo) += (double) dDelta;
		++myPtr;
		locNo >>= 1;
	}


	return rate;
}

double RateTree::getRate(int locNo) {
	
	return rates[locNo];

}

double RateTree::getTotalRate() {
	//Top of tree:
	// MC: 26-4-2014
	// return rates[2*nPaddedLength-2];
        return (double) ldTotalRate;
}

int RateTree::scrubRates() {
	double *fenA = &rates[0];

	int nMax = 2*nPaddedLength-1;
	for(int j=0; j<nMax; j++) {
		*fenA++ = 0.0;
	}

	// MC: 26-4-2014
	ldTotalRate = 0.0;

	return 1;
}

int RateTree::getLocationEventIndex(double rate) {
	return treeSearch(rate);
}

int RateTree::getWorkToResubmit() {
	return nTreeLevels;
}

double RateTree::submitRate_dumb(int locNo, double rate) {

	double **myPtr = &treeLevels[1];

	*(*myPtr+locNo) = rate;

	return rate;
}

int RateTree::fullResum() {

	int nLevelLength = nPaddedLength/2;
	for(int i=2; i<=nTreeLevels; i++) {
		for(int j=0; j<nLevelLength; j++) {
			treeLevels[i][j] = treeLevels[i-1][2*j] + treeLevels[i-1][2*j + 1];
		}
		nLevelLength /= 2;
	}

	ldTotalRate = rates[2*nPaddedLength-2];

	return 0;
}

int RateTree::treeSearch(double rate) {
	double **myPtr = &treeLevels[nTreeLevels-1];

	int nFIndex = 0;

	while(*myPtr != NULL) {
		//Multiplying by 2 at start instead of end as we always start at nFIndex=0,
		//and we don't want it to change after we've found it and myPtr becomes null...
		nFIndex *= 2;

		double dLeftRate = *(*myPtr + nFIndex);
		if(dLeftRate <= rate) {
			++nFIndex;
			rate -= dLeftRate;
		}

		--myPtr;
	}

	return nFIndex;
}
