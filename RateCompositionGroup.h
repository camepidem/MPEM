//RateCompositionGroup.h

#ifndef RATECOMPOSITIONGROUP_H
#define RATECOMPOSITIONGROUP_H 1

#include "RateStructure.h"

class RateCompositionRejection;

struct SGroupIndexStorage {
	int iIndex;
};

struct SEventDataStore {
	int iEvent;
	double dRate;
};

class RateCompositionGroup : public RateStructure {

public:

	//Constructor/destructor:
	RateCompositionGroup(RateCompositionRejection *ParentRateCR, int size, int iIndex, double dMinRate, double dMaxRate);
	~RateCompositionGroup();

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

	int fillContiguousZeroRates(int nRatesToFill);

	//Find approx number of operations to resubmit all rates (used when landscape needs to be cleared; may be easier to just manually set all rates to 0.0)
	int getWorkToResubmit();

	//Input to array without automatically summing, useful for resubmitting large portion of tree, must be followed by a fullResum() before tree is used
	double submitRate_dumb(int locNo, double rate);

	//Forces recalculation of whole tree from bottom up
	int fullResum();

	int insertElementToGroup(int locNo, double dRate);
	int removeElementFromGroup(int locNo);

protected:

	//Position in global structure:
	RateCompositionRejection *pParentRateCR;
	int iGroupIndex;

	//Bounds of RateGroup:
	double dMinRate, dMaxRate;
	int nEventsMax;

	//Current State:
	double dTotalRate;

        // MC: 30-4-2014
        long double ldTotalRate;

	int nEventsActive;

	//Current ordering of event storage:
	vector<SGroupIndexStorage> pSLocNoToIndexMap;

	//Storage of events: Location of event and rate of event
	vector<SEventDataStore> pSEvents;

};

#endif


