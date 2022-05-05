//RateCompositionRejection.cpp

#include "stdafx.h"
#include "RateCompositionGroup.h"
#include "RateCompositionRejection.h"
#include "RateTree.h"
#include "myRand.h"

#pragma warning(disable : 4996)

RateCompositionRejection::RateCompositionRejection(int size, double d_MinRate, double d_MaxRate) {

	//Memory housekeeping:
	groupRates						= NULL;

	//Initialisation:
	nEventsStored = size;

	double dLogMinRate = log(d_MinRate);
	if(dLogMinRate >= 0.0) {
		iOffsetMinRate = int(dLogMinRate);
	} else {
		iOffsetMinRate = int(dLogMinRate) - 1;//C++ rounds towards zero, so need to correct offset
	}
	
	dMinRate = pow(2.0, iOffsetMinRate);
	
	double dLogMaxRate = log(d_MaxRate);
	int iOffsetMaxRate;
	if(dLogMaxRate >= 0.0) {
		iOffsetMaxRate = int(dLogMaxRate);
	} else {
		iOffsetMaxRate = int(dLogMaxRate)-1;//C++ rounds towards zero, so need to correct offset
	}

	dMaxRate = pow(2.0, iOffsetMaxRate);

	nGroupsNormal = (iOffsetMaxRate - iOffsetMinRate + 1);

	nGroups = nGroupsNormal;

	iGroupEpsilon = nGroups++;
	iGroupOmega = nGroups++;
	iGroupZero = nGroups++;

	nGroupsSpecial = nGroups - nGroupsNormal;

	groupRates = new RateTree(nGroups);

	groups.resize(nGroups);

	int nFactor = 1;
	for(int iGroup = 0; iGroup<nGroupsNormal; iGroup++) {
		double dLowerRate = dMinRate*nFactor;
		groups[iGroup] = new RateCompositionGroup(this, nEventsStored, iGroup, dLowerRate, dLowerRate*2);
		nFactor *= 2;
	}

	groups[iGroupEpsilon] = new RateCompositionGroup(this, nEventsStored, iGroupEpsilon, 0.0, dMinRate);

	groups[iGroupOmega] = new RateCompositionGroup(this, nEventsStored, iGroupOmega, dMaxRate, dMaxRate*32);//TODO: Make one of these that dynamically changes lower/upper bounds...
	
	groups[iGroupZero] = new RateCompositionGroup(this, nEventsStored, iGroupZero, 0.0, 0.0);

	pSLocationNumberToStorageMap.resize(nEventsStored);
	
	//Start all events at rate zero:
	scrubRates();

}

RateCompositionRejection::~RateCompositionRejection() {

	if(groups.size() != 0) {
		for(int iGroup=0; iGroup<nGroups; iGroup++) {
			delete groups[iGroup];
		}
	}

	if(groupRates != NULL) {
		delete groupRates;
		groupRates = NULL;
	}

}

int RateCompositionRejection::nGetGroupIDFromRate(double dRate) {
	
	if(dRate > 0.0) {
		if(dRate >= dMinRate) {
			if(dRate < dMaxRate) {

				double dLogRate = log(dRate);//stupid rounding...
				if(dLogRate < 0.0) {
					dLogRate -= 1.0;
				}
				return int(dLogRate)-iOffsetMinRate;

			}  else {
				return iGroupOmega;
			}
		} else {
			return iGroupEpsilon;
		}
	} else {
		return iGroupZero;
	}
	
	return -1;
}

double RateCompositionRejection::submitRate(int locNo, double rate) {
	
	int iTargetGroup = nGetGroupIDFromRate(rate);

	int iCurrentGroup = pSLocationNumberToStorageMap[locNo].iGroup;
	if(iCurrentGroup != iTargetGroup) {
		groups[iCurrentGroup]->removeElementFromGroup(locNo);
		groups[iTargetGroup]->insertElementToGroup(locNo, rate);
		pSLocationNumberToStorageMap[locNo].iGroup = iTargetGroup;
	} else {
		groups[iTargetGroup]->submitRate(locNo, rate);
	}

	return rate;
}

double RateCompositionRejection::getRate(int locNo) {
	return groups[pSLocationNumberToStorageMap[locNo].iGroup]->getRate(locNo);
}

double RateCompositionRejection::getTotalRate() {
	return groupRates->getTotalRate();
}

//Remove everything from its current group and put it in the zeros group at zero rate:
int RateCompositionRejection::scrubRates() {

        // MC: 21-4-2014
	// for(int iGroup=0; iGroup<nGroupsNormal; iGroup++) {
	for(int iGroup=0; iGroup<nGroups; iGroup++) {
		groups[iGroup]->scrubRates();
	}

	groups[iGroupZero]->fillContiguousZeroRates(nEventsStored);

	groupRates->scrubRates();

	for(int iEvent=0; iEvent<nEventsStored; iEvent++) {
		pSLocationNumberToStorageMap[iEvent].iGroup = iGroupZero;
	}

	return 1;
}

//Find approx number of operations to resubmit all rates (used when landscape needs to be cleared; may be easier to just manually set all rates to 0.0)
int RateCompositionRejection::getWorkToResubmit() {
	return 1;
}

//Input to array without automatically summing, useful for resubmitting large portion of tree, must be followed by a fullResum() before tree is used
double RateCompositionRejection::submitRate_dumb(int locNo, double rate) {
	return submitRate(locNo, rate);
}

//Forces recalculation of whole tree from bottom up
int RateCompositionRejection::fullResum() {
	//Unnecessary for this method
	return 1;
}

int RateCompositionRejection::subGroupReportRate(int iGroup, double dRate) {

	groupRates->submitRate(iGroup, dRate);

	return 1;
}

//Find location for given marginal rate 
int RateCompositionRejection::getLocationEventIndex(double rate) {

        // MC: 3-6-2014
        double prevGroupTotalRates = 0.0;
        double groupTotalRate;

	//Select composition group:
	//TODO: Should group rates live in cleverer structure?
	int iSelectedGroup;
	double dMarginalRate = rate*dUniformRandom();

        // cout << " dMarginalRate = " << dMarginalRate << endl;
	for(iSelectedGroup=0; iSelectedGroup<nGroups; iSelectedGroup++) {
                // cout << " i = " << iSelectedGroup << "; gTR = ";
                // cout << groups[iSelectedGroup]->getTotalRate() << endl;
                groupTotalRate = groups[iSelectedGroup]->getTotalRate();

		if(rate < groupTotalRate + prevGroupTotalRates) {
                        // cout << " FOUND! Got i = " << iSelectedGroup << endl;
			return groups[iSelectedGroup]->getLocationEventIndex(rate);
		}
                else {
                        prevGroupTotalRates += groupTotalRate;
                }
	}

        // Technically, if we got this far then we've failed. However,
        // it could be due to rounding errors off the high end.
	return -1;

}
