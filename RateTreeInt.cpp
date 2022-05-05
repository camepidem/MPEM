//RateTreeInt.cpp

#include "stdafx.h"
#include "RateTreeInt.h"

#pragma warning(disable : 4996)

RateTreeInt::RateTreeInt(int size) {

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

RateTreeInt::~RateTreeInt() {

}

double RateTreeInt::submitRate(int locNo, double rate) {
	//Interface for a location to submit a rate for an event to the world event managing structure
	//Handles storage of rate and manages tree structure

	int **myPtr = &treeLevels[1];

	int dDelta = int(rate - *(*myPtr+locNo));

	while(*myPtr != NULL) {
		*(*myPtr + locNo) += dDelta;
		++myPtr;
		locNo >>= 1;
	}


	return rate;
}

double RateTreeInt::getRate(int locNo) {
	
	return (double)rates[locNo];

}

double RateTreeInt::getTotalRate() {
	//Top of tree:
	return (double)rates[2*nPaddedLength - 2];
}

int RateTreeInt::scrubRates() {
	int *fenA = &rates[0];

	int nMax = 2*nPaddedLength-1;
	for(int j=0; j<nMax; j++) {
		*fenA++ = 0;
	}

	return 1;
}

int RateTreeInt::getLocationEventIndex(double rate) {
	return treeSearch(rate);
}

int RateTreeInt::getWorkToResubmit() {
	return nTreeLevels;
}

double RateTreeInt::submitRate_dumb(int locNo, double rate) {

	int **myPtr = &treeLevels[1];

	*(*myPtr+locNo) = int(rate);

	return rate;
}

int RateTreeInt::fullResum() {

	int nLevelLength = nPaddedLength/2;
	for(int i=2; i<=nTreeLevels; i++) {
		for(int j=0; j<nLevelLength; j++) {
			treeLevels[i][j] = treeLevels[i-1][2*j] + treeLevels[i-1][2*j + 1];
		}
		nLevelLength /= 2;
	}

	return 0;
}

int RateTreeInt::treeSearch(double rate) {
	int **myPtr = &treeLevels[nTreeLevels-1];

	int nFIndex = 0;

	while(*myPtr != NULL) {
		//Multiplying by 2 at start instead of end as we always start at nFIndex=0,
		//and we don't want it to change after we've found it and myPtr becomes null...
		nFIndex *= 2;

		int dLeftRate = *(*myPtr + nFIndex);
		if(dLeftRate <= rate) {
			++nFIndex;
			rate -= dLeftRate;
		}

		--myPtr;
	}

	return nFIndex;
}
