//RateTreeInt.h

#ifndef RATETREEINT_H
#define RATETREEINT_H 1

#include "RateStructure.h"

class RateTreeInt : public RateStructure {

public:

	//Constructor/destructor:
	RateTreeInt(int size);
	~RateTreeInt();

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

	int treeSearch(double rate);

	//Rate Structure Data:
	vector<int> rates;					//array holding tree rate sums for each event
	int nPaddedLength;					//tree needs data of length 2^n, so this is the smallest 2^n >= nLAct

	int nTreeLevels;					//Number of levels in the tree
	vector<int*> treeLevels;			//Pointers to the start of each level in array (padded so that level 0 and level nLevels+1 are NULL)

};

#endif
