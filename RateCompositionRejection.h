//RateCompositionRejection.h

#ifndef RATECOMPOSITIONREJECTION_H
#define RATECOMPOSITIONREJECTION_H 1

#include "RateStructure.h"

class RateTree;
class RateCompositionGroup;

struct SIndexStorage {

	int iGroup;

};

class RateCompositionRejection : public RateStructure {

public:

	//Constructor/destructor:
	RateCompositionRejection(int size, double dMinRate, double dMaxRate);
	~RateCompositionRejection();

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

	//If a subgroup changes the ordering it reports to this structure to update the scheme to find the 
	//int reportEventPosition(int index, int group);

	int subGroupReportRate(int iGroup, double dRate);

protected:

	//int *pnLocationNumberToObjectIdMap;
	int nEventsStored;
	vector<SIndexStorage> pSLocationNumberToStorageMap;

	int iOffsetMinRate;
	double dMinRate, dMaxRate;
	
	//double dTotalRate;
	//double *groupRates;// OR ...
	RateTree *groupRates;
	//RateCompositionGroup *groupGroup;//If there are a large number of groups, wasteful to pick between them by normal sampling methods 
	//use yet another level of CR sampling or perhaps binary tree
	
	int nGetGroupIDFromRate(double dRate);

	int nGroups;// standard logarithmically binned groups + zeros, epsilons and omegas
	int nGroupsNormal;
	int nGroupsSpecial;
	vector<RateCompositionGroup *> groups;

	int iGroupZero;
	RateCompositionGroup *groupZeros;//Storage of the events at rate zero

	double epsilonRate;// Rates below minRate but non zero are added to the special epsilonGroup - avoids having inordinate numbers of groups
	int iGroupEpsilon;
	RateCompositionGroup *groupEpsilon;//Fallback group for when events have non zero rate below minRate TODO: Make this a TREE?
	int iGroupOmega;
	RateCompositionGroup *groupOmega;//Fallback group for when events have rate above maxRate TODO: Make this a TREE?

};

#endif
