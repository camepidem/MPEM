//RateCompositionGroup.cpp

#include "stdafx.h"
#include "myRand.h"
#include "RateCompositionRejection.h"
#include "RateCompositionGroup.h"

#pragma warning(disable : 4996)

RateCompositionGroup::RateCompositionGroup(RateCompositionRejection *ParentRateCR, int size, int iIndex, double d_MinRate, double d_MaxRate) {

	pParentRateCR = ParentRateCR;

	nEventsMax = size;

	iGroupIndex = iIndex;

	dMinRate = d_MinRate;
	dMaxRate = d_MaxRate;

	pSLocNoToIndexMap.resize(nEventsMax);
	pSEvents.resize(nEventsMax);

	scrubRates();

}

RateCompositionGroup::~RateCompositionGroup() {

}

double RateCompositionGroup::submitRate(int locNo, double rate) {

	SEventDataStore &rEvent = pSEvents[pSLocNoToIndexMap[locNo].iIndex];

	double dOldRate = rEvent.dRate;
	rEvent.dRate = rate;

	// MC: 30-4-2014
	ldTotalRate -= (long double)dOldRate;
	ldTotalRate += (long double)rate;
	// dTotalRate += rate - dOldRate;

	// pParentRateCR->subGroupReportRate(iGroupIndex, dTotalRate);
	pParentRateCR->subGroupReportRate(iGroupIndex, (double)ldTotalRate);

	return rate;
}

int RateCompositionGroup::insertElementToGroup(int locNo, double dRate) {

	//Stick in the new event rate at the end
	pSEvents[nEventsActive].iEvent = locNo;
	pSEvents[nEventsActive].dRate = dRate;

	//log where it went
	pSLocNoToIndexMap[locNo].iIndex = nEventsActive;

	//increment size of active array
	nEventsActive++;

        // MC: 30-4-2014
	//Add rate to total:
	//dTotalRate += dRate;
        ldTotalRate += dRate;

	// pParentRateCR->subGroupReportRate(iGroupIndex, dTotalRate);
	pParentRateCR->subGroupReportRate(iGroupIndex, (double) ldTotalRate);

	return 1;
}

int RateCompositionGroup::removeElementFromGroup(int locNo) {
	
	//Find the element being removed:
	int iRemovalElement = pSLocNoToIndexMap[locNo].iIndex;

	SEventDataStore &rEvent = pSEvents[iRemovalElement];

	double dRateRemovalElement = rEvent.dRate;

	//Move the last element here:
	int iPreviousLastEvent = pSEvents[nEventsActive-1].iEvent;
	pSLocNoToIndexMap[iPreviousLastEvent].iIndex = iRemovalElement;//Log as in new place
	//Have vacated slot take on new values:
	rEvent.iEvent = iPreviousLastEvent;
	rEvent.dRate = pSEvents[nEventsActive-1].dRate;

	//Decrement the length of the group:
	nEventsActive--;

        // MC: 30-4-2014
	//Update total rate:
	//dTotalRate -= dRateRemovalElement;
	ldTotalRate -= (long double) dRateRemovalElement;

	// pParentRateCR->subGroupReportRate(iGroupIndex, dTotalRate);
	pParentRateCR->subGroupReportRate(iGroupIndex, (double) ldTotalRate);

	return 1;
}

//Retrieve rate for given location:
double RateCompositionGroup::getRate(int locNo) {
	return pSEvents[pSLocNoToIndexMap[locNo].iIndex].dRate;
}

//Find total rate of event:
double RateCompositionGroup::getTotalRate() {
        // MC: 30-4-2014
	//return dTotalRate;
	return (double) ldTotalRate;
}

//Find location for given marginal rate 
int RateCompositionGroup::getLocationEventIndex(double rate) {

	//Do actual CR sampling:
	int iSelectedEvent = -1;
	
	int iRandomIndex;
	double dRandomRate;

	while(iSelectedEvent < 0) {
		dRandomRate = dMaxRate*dUniformRandom();
		iRandomIndex = nUniformRandom(0, nEventsActive-1);

		if(dRandomRate < pSEvents[iRandomIndex].dRate) {
			iSelectedEvent = pSEvents[iRandomIndex].iEvent;
		}
	}

	return iSelectedEvent;
}

//cleans out all rates to 0.0 (part of interventions)
int RateCompositionGroup::scrubRates() {

	dTotalRate = 0.0;
	nEventsActive = 0;

        // MC: 30-4-2014
        ldTotalRate = 0.0;

	pParentRateCR->subGroupReportRate(iGroupIndex, dTotalRate);

	//Don't need to change the arrays as they will be repopulated when they come back into scope
	//Although inevitably I will come back to here when bugs cause old data to be read

	return 1;
}

int RateCompositionGroup::fillContiguousZeroRates(int nRatesToFill) {

	dTotalRate = 0.0;
	nEventsActive = nRatesToFill;

        // MC: 30-4-2014
        ldTotalRate = 0.0;

	for(int iEvent=0; iEvent<nRatesToFill; iEvent++) {
		//Allocate as a zero rate:
		pSEvents[iEvent].iEvent = iEvent;
		pSEvents[iEvent].dRate = 0.0;

		//log where it went
		pSLocNoToIndexMap[iEvent].iIndex = iEvent;
	}

	pParentRateCR->subGroupReportRate(iGroupIndex, dTotalRate);

	return 1;
}

//Find approx number of operations to resubmit all rates (used when landscape needs to be cleared; may be easier to just manually set all rates to 0.0)
int RateCompositionGroup::getWorkToResubmit() {
	return 1;
}

//Input to array without automatically summing, useful for resubmitting large portion of tree, must be followed by a fullResum() before tree is used
double RateCompositionGroup::submitRate_dumb(int locNo, double rate) {
	return submitRate(locNo, rate);
}

//Forces recalculation of whole tree from bottom up
int RateCompositionGroup::fullResum() {
	return 1;
}
