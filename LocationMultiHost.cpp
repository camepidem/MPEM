//LocationMultiHost.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "ListManager.h"
#include "LinkedList.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "SummaryDataManager.h"
#include "RateManager.h"
#include "myRand.h"
#include "LocationMultiHost.h"

Landscape* LocationMultiHost::world;
int LocationMultiHost::nPopulations;
LinkedList<LocationMultiHost> **LocationMultiHost::pLocationLists;

//Static Buffers:
vector<double> LocationMultiHost::pdDensityBuffer;
vector<double *> LocationMultiHost::pdHostAllocationsBuffer;

//Constructor(s):
LocationMultiHost::LocationMultiHost(int nX, int nY) {
	
	xPos = nX;
	yPos = nY;

	actionFlag = 0;

	//Lists:
	pLists.resize(ListManager::nLists);

	for(int i=0; i<ListManager::nLists; i++) {
		pLists[i] = NULL;
	}

}

//Destructor:
LocationMultiHost::~LocationMultiHost() {

	for (int i = 0; i < nPopulations; i++) {
		if (populations[i] != NULL) {
			delete populations[i];
			populations[i] = NULL;
		}
	}

}

//Initialization:
int LocationMultiHost::setLandscape(Landscape *myWorld) {
	world = myWorld;
	return 1;
}

int LocationMultiHost::initLocations() {

	int nTotalClasses = 0;

	nPopulations = PopulationCharacteristics::nPopulations;

	pdDensityBuffer.resize(nPopulations);
	pdHostAllocationsBuffer.resize(nPopulations);//Dont allocate individual arrays, 
														//these will be allocated by populations returning as pointers

	for(int i=0; i<nPopulations; i++) {
		nTotalClasses += Population::pGlobalPopStats[i]->nClasses;
	}

	return 1;
}

std::string LocationMultiHost::debugGetStateString()
{
	std::stringstream os;

	os << "Location" << std::endl;
	os << "x = " << xPos << std::endl;
	os << "y = " << yPos << std::endl;
	os << "Populations" << std::endl;
	for (int iPop = 0; iPop < nPopulations; ++iPop) {
		os << "Population " << iPop << std::endl;
		if (populations[iPop] == NULL) {
			os << "NULL" << std::endl;
		} else {
			os << populations[iPop]->debugGetStateString();
		}
	}

	return os.str();
}

int LocationMultiHost::readDensities(double *pdDensities) {
	
	double dErrorTolerance = 1e-6;

	//Check Data reading:
	double dTotal = 0.0;
	for(int i=0; i<nPopulations; i++) {
		if(pdDensities[i] > 0.0) {
			if(pdDensities[i] <= 1.0 + dErrorTolerance) {
				dTotal += pdDensities[i];
			} else {
				reportReadError("ERROR: At x=%d, y=%d In population %d Host density %f > 1.0\n",xPos,yPos,i,pdDensities[i]);
				return 0;
			}
		}
	}

	if(dTotal > 1.0 + dErrorTolerance) {
		reportReadError("ERROR: At x=%d, y=%d Total host density=%f > 1.0\n",xPos,yPos,dTotal);
		return 0;
	}

	//Create populations:
	populations.resize(nPopulations);
	for(int i=0; i<nPopulations; i++) {
		populations[i] = NULL;
	}

	for(int i=0; i<nPopulations; i++) {
		if(pdDensities[i] > 0.0) {
			populations[i] = new Population(this, i, pdDensities[i]);
			joinList(ListManager::iActiveList);
		}
	}
	
	return 1;
}

double *LocationMultiHost::getDensities() {
	
	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL) {
			pdDensityBuffer[i] = populations[i]->getDensity();
		} else {
			pdDensityBuffer[i] = 0.0;
		}
	}
	
	return &pdDensityBuffer[0];
}

int LocationMultiHost::populate(double **pdHostAllocations) {
	
	//Populations will automatically create clean populations if provided with null initial data:
	if(populations.size() != 0) {//Check has any populations at all:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {//If population exists, pass data:
				populations[i]->populate(pdHostAllocations[i]);
			}
		}
	}
	return 1;
}

double **LocationMultiHost::getHostAllocations() {
	
	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL) {
			pdHostAllocationsBuffer[i] = populations[i]->getHostAllocation();
		} else {
			pdHostAllocationsBuffer[i] = NULL;
		}
	}
	
	return &pdHostAllocationsBuffer[0];
}

double LocationMultiHost::getInfectiousness(int iPop) {
	if(populations[iPop] != NULL) {
		return populations[iPop]->getInfectiousness();
	} else {
		return 0.0;
	}
}

int LocationMultiHost::setInfBase(double dInf, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->setInfBase(dInf);
		}
	}
	
	return 1;
}

int LocationMultiHost::setSusBase(double dSus, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->setSusBase(dSus);
		}
	}
	
	return 1;
}

int LocationMultiHost::scaleInfBase(double dInf, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->scaleInfBase(dInf);
		}
	}
	
	return 1;
}

int LocationMultiHost::scaleSusBase(double dSus, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->scaleSusBase(dSus);
		}
	}
	
	return 1;
}

int LocationMultiHost::setInfSpray(double dInf, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->setInfSpray(dInf);
		}
	}
	
	return 1;
}

int LocationMultiHost::setSusSpray(double dSus, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->setSusSpray(dSus);
		}
	}
	
	return 1;
}

double LocationMultiHost::getInfSpray(int iPop) {
	if(populations[iPop] != NULL) {
		return populations[iPop]->getInfSpray();
	} else {
		return 1.0;
	}
}

double LocationMultiHost::getSusSpray(int iPop) {
	if(populations[iPop] != NULL) {
		return populations[iPop]->getSusSpray();
	} else {
		return 1.0;
	}
}

int LocationMultiHost::setSearch(int bNewSearch, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->setSearch(bNewSearch != 0);
		}
	}
	
	return 1;
}

int LocationMultiHost::setKnown(int bNewKnown, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			populations[i]->setKnown(bNewKnown != 0);
		}
	}
	
	return 1;
}

bool LocationMultiHost::getSearch(int iPop) {

	if(iPop < 0) {
		//Try all populations, see if any are searched:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				if(populations[i]->getSearch()) {
					return true;
				}
			}
		}
		//Didn't find any populations that are searched:
		return false;
	}

	if(populations[iPop] != NULL) {
		return populations[iPop]->getSearch();
	} else {
		return false;
	}
}

bool LocationMultiHost::getKnown(int iPop) {

	if(iPop < 0) {
		//Try all populations, see if any are known:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				if(populations[i]->getKnown()) {
					return true;
				}
			}
		}
		//Didn't find any populations that are known:
		return false;
	}

	if(populations[iPop] != NULL) {
		return populations[iPop]->getKnown();
	} else {
		return false;
	}
}

int LocationMultiHost::resetDetectionStatus() {
	
	//TODO:
	
	return 1;
}

double LocationMultiHost::getCoverage(int iPop) {

	double dTotalCoverage = 0.0;

	if(iPop < 0) {
		//Try all populations, see if any are known:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				dTotalCoverage += populations[i]->getDensity();
			}
		}
		//Didn't find any populations that are known:
		return dTotalCoverage;
	}

	if(populations[iPop] != NULL) {
		dTotalCoverage += populations[iPop]->getDensity();
	}

	return dTotalCoverage;

}

double LocationMultiHost::getSusceptibleCoverageProportion(int iPop) {

	double dTotalCoverage = 0.0;

	if(iPop < 0) {
		//Try all populations, see if any are known:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				double dPopCoverage = populations[i]->getSusceptibleCoverage();
				dTotalCoverage += dPopCoverage;
			}
		}
		//Didn't find any populations that are known:
		return dTotalCoverage;
	}

	if(populations[iPop] != NULL) {
		double dPopCoverage = populations[iPop]->getSusceptibleCoverage();
		dTotalCoverage += dPopCoverage;
	}

	return dTotalCoverage;

}

double LocationMultiHost::getInfectiousCoverageProportion(int iPop) {

	double dTotalCoverage = 0.0;

	if(iPop < 0) {
		//Try all populations, see if any are known:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				dTotalCoverage += populations[i]->getDensity()*populations[i]->getInfectiousFraction();
			}
		}
		//Didn't find any populations that are known:
		return dTotalCoverage;
	}

	if(populations[iPop] != NULL) {
		dTotalCoverage += populations[iPop]->getDensity()*populations[iPop]->getInfectiousFraction();
	}

	return dTotalCoverage;

}

double LocationMultiHost::getDetectableCoverageProportion(int iPop) {

	double dTotalCoverage = 0.0;

	if(iPop < 0) {
		//Try all populations, see if any are known:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				dTotalCoverage += populations[i]->getDensity()*populations[i]->getDetectableFraction();
			}
		}
		//Didn't find any populations that are known:
		return dTotalCoverage;
	}

	if(populations[iPop] != NULL) {
		dTotalCoverage += populations[iPop]->getDensity()*populations[iPop]->getDetectableFraction();
	}

	return dTotalCoverage;

}

double LocationMultiHost::getSusceptibilityWeightedCoverage(int iPop) {

	double dTotalCoverage = 0.0;

	if(iPop < 0) {
		//Try all populations, see if any are known:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				dTotalCoverage += populations[i]->getDensity()*populations[i]->getSusceptibility();
			}
		}
		//Didn't find any populations that are known:
		return dTotalCoverage;
	}

	if(populations[iPop] != NULL) {
		dTotalCoverage += populations[iPop]->getDensity()*populations[iPop]->getSusceptibility();
	}

	return dTotalCoverage;

}

double LocationMultiHost::cull(double effectiveness, double dMaxAllowedHost, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	double dReturnValue = 0.0;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			dReturnValue += populations[i]->cull(effectiveness, dMaxAllowedHost);
		}
	}
	
	return dReturnValue;
}

double LocationMultiHost::rogue(double effectiveness, double dMaxAllowedHost, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	double dReturnValue = 0.0;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			dReturnValue += populations[i]->rogue(effectiveness, dMaxAllowedHost);
		}
	}
	
	return dReturnValue;
}

double LocationMultiHost::sprayAS(double effectiveness, double dMaxAllowedHost, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	double dReturnValue = 0.0;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			dReturnValue += populations[i]->sprayAS(effectiveness, dMaxAllowedHost);
		}
	}
	
	return dReturnValue;
}

double LocationMultiHost::sprayPR(double effectiveness, double dMaxAllowedHost, int iPop) {
	
	int iMin = iPop;
	int iMax = iPop+1;

	double dReturnValue = 0.0;

	if(iPop < 0) {
		iMin = 0;
		iMax = nPopulations;
	}

	for(int i=iMin; i<iMax; i++) {
		if(populations[i] != NULL) {
			dReturnValue += populations[i]->sprayPR(effectiveness, dMaxAllowedHost);
		}
	}
	
	return dReturnValue;
}

void LocationMultiHost::subPopulationLeftList(int iList) {
	//If there are no subpopulations on that list, leave it:
	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL && populations[i]->pLists[iList] != NULL) {
			return;
		}
	}

	leaveList(iList);

	return;
}

void LocationMultiHost::subPopulationJoinedList(int iList) {

	//Already prevents joining something you shouldn't:
	joinList(iList);

	return;
}

void LocationMultiHost::joinList(int iList) {

	if(pLists[iList] == NULL) {
		pLists[iList] = pLocationLists[iList]->addAsFirst(this);
	}

	return;
}

void LocationMultiHost::leaveList(int iList) {

	if(pLists[iList] != NULL) {
		pLists[iList] = pLocationLists[iList]->excise(pLists[iList]);
	}

	return;
}

void LocationMultiHost::subPopulationFirstInfected(int iPop) {

	SummaryDataManager::submitFirstInfection(iPop, xPos, yPos);

	//See if it was the only population so far infected:
	for (int i = 0; i < nPopulations; i++) {
		if (populations[i] != NULL && i != iPop) {
			if (populations[i]->getEpidemiologicalMetric(PopulationCharacteristics::timeFirstInfection) >= 0.0) {
				//Something else already infected, so dont report as the locations first infection:
				return;
			}
		}
	}

	//Nothing else already infected, so report as the locations first infection:
	SummaryDataManager::submitFirstInfection(-1, xPos, yPos);

	return;
}

//Special case as when populating not all populations exist yet
void LocationMultiHost::subPopulationInitiallyInfected(int iPop) {

	SummaryDataManager::submitFirstInfection(iPop, xPos, yPos);
	SummaryDataManager::submitCurrentInfection(iPop, xPos, yPos);

	//See if it was the only population so far infected:
	for(int i=0; i<iPop; i++) {
		if(populations[i] != NULL && i != iPop) {
			if(populations[i]->getEpidemiologicalMetric(PopulationCharacteristics::timeFirstInfection) >= 0.0) {
				//Something else already infected, so dont report as the locations first infection:
				return;
			}
		}
	}

	//Nothing else already infected, so report as the locations first infection:
	SummaryDataManager::submitFirstInfection(-1, xPos, yPos);
	SummaryDataManager::submitCurrentInfection(-1, xPos, yPos);

	return;
}

void LocationMultiHost::subPopulationCurrentInfection(int iPop) {

	SummaryDataManager::submitCurrentInfection(iPop, xPos, yPos);

	//See if it was the only population so far infected:
	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL && i != iPop) {
			if(populations[i]->getCurrentlyInfectedFraction() > 0.0) {
				//Something else already infected, so don't report as the overall locations gain of currently infected status:
				return;
			}
		}
	}

	//Nothing else currently infected, so report as the location now currently infected:
	SummaryDataManager::submitCurrentInfection(-1, xPos, yPos);

	return;
}

void LocationMultiHost::subPopulationLossOfCurrentInfection(int iPop) {

	SummaryDataManager::submitCurrentLossOfInfection(iPop, xPos, yPos);

	//See if it was the only population left infected:
	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL && i != iPop) {
			if(populations[i]->getCurrentlyInfectedFraction() > 0.0) {
				//Something else still infected, so don't report as the overall locations loss of infection:
				return;
			}
		}
	}

	//Nothing else already infected, so report as the locations first infection:
	SummaryDataManager::submitCurrentLossOfInfection(-1, xPos, yPos);

	return;
}

//Special case for cleaning populations: the location should only consider those sub-populations which are not currently in invalid state during scrubbing, as with the initial infection function
void LocationMultiHost::subPopulationInitialLossOfCurrentInfection(int iPop) {

	SummaryDataManager::submitCurrentLossOfInfection(iPop, xPos, yPos);

	return;
}

void LocationMultiHost::subPopulationInitialGainOfCurrentInfection(int iPop) {

	SummaryDataManager::submitCurrentInfection(iPop, xPos, yPos);

	return;
}

void LocationMultiHost::subPopulationCurrentSusceptible(int iPop) {

	SummaryDataManager::submitCurrentSusceptibles(iPop, xPos, yPos);

	//See if it was the only population so far susceptible:
	for (int i = 0; i<nPopulations; i++) {
		if (populations[i] != NULL && i != iPop) {
			if (populations[i]->getSusceptibleCoverage() > 0.0) {
				//Something else already susceptible, so don't report as the overall locations gain of currently susceptible status:
				return;
			}
		}
	}

	//Nothing else currently susceptible, so report as the location now currently susceptible:
	SummaryDataManager::submitCurrentSusceptibles(-1, xPos, yPos);

	return;
}

void LocationMultiHost::subPopulationLossOfCurrentSusceptible(int iPop) {

	SummaryDataManager::submitCurrentLossOfSusceptibles(iPop, xPos, yPos);

	//See if it was the only population left susceptible:
	for (int i = 0; i<nPopulations; i++) {
		if (populations[i] != NULL && i != iPop) {
			if (populations[i]->getSusceptibleCoverage() > 0.0) {
				//Something else still susceptible, so don't report as the overall locations loss of susceptibility:
				return;
			}
		}
	}

	//Nothing else already infected, so report as the locations first infection:
	SummaryDataManager::submitCurrentLossOfSusceptibles(-1, xPos, yPos);

	return;
}

//Need to have a way to get epidemiological metrics for all subpopulations
//For Severity probably want weighted sum - note don't know that the times are actually the same for each subpopulations max, so this could be meaningless
//For TimeFirstInfection probably want min - time first infection for a location is the time at which any subpopulation was infected
double LocationMultiHost::getEpidemiologicalMetric(int iPop, int iMetric) {

	double dMetric;
	bool bMetricDefined = false;

	if(iPop >= 0) {
		if(populations[iPop] != NULL) {
			dMetric = populations[iPop]->getEpidemiologicalMetric(iMetric);
		} else {
			switch (iMetric) {
				case PopulationCharacteristics::timeFirstInfection:
				case PopulationCharacteristics::timeFirstSymptomatic:
				case PopulationCharacteristics::timeFirstRemoved:
					dMetric = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
					break;
				default:
					dMetric = -1.0;
					break;
			}
		}
	} else {
		switch(iMetric) {
		case PopulationCharacteristics::severity:
			{
				double dTotalDensity = 0.0;
				for(int iSubPop=0; iSubPop<PopulationCharacteristics::nPopulations; iSubPop++) {
					if(populations[iSubPop] != NULL) {
						if(!bMetricDefined) {
							bMetricDefined = true;
							dMetric = 0.0;
						}
						double dSubPopSeverity = populations[iSubPop]->getEpidemiologicalMetric(iMetric);
						double dSubPopDensity = populations[iSubPop]->getDensity();
						dTotalDensity += dSubPopDensity;
						dMetric += dSubPopSeverity*dSubPopDensity;
					}
				}
				if(bMetricDefined && dTotalDensity > 0.0) {
					dMetric /= dTotalDensity;
				} else {
					dMetric = -1.0;//TODO: Magic undefined value
				}
			}
			break;
		case PopulationCharacteristics::timeFirstInfection:
			dMetric = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
			for(int iSubPop=0; iSubPop<PopulationCharacteristics::nPopulations; iSubPop++) {
				if (populations[iSubPop] != NULL) {
					double dSubPopTimeFirstinfection = populations[iSubPop]->getEpidemiologicalMetric(iMetric);
					if (!bMetricDefined) {
						bMetricDefined = true;
						dMetric = dSubPopTimeFirstinfection;
					}
					if(dSubPopTimeFirstinfection < dMetric) {
						dMetric = dSubPopTimeFirstinfection;
					}
				}
			}
			break;
		case PopulationCharacteristics::timeFirstSymptomatic:
			dMetric = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
			for (int iSubPop = 0; iSubPop<PopulationCharacteristics::nPopulations; iSubPop++) {
				if (populations[iSubPop] != NULL) {
					double dSubPopTimeFirstSymptoms = populations[iSubPop]->getEpidemiologicalMetric(iMetric);
					if (!bMetricDefined) {
						bMetricDefined = true;
						dMetric = dSubPopTimeFirstSymptoms;
					}
					if (dSubPopTimeFirstSymptoms < dMetric) {
						dMetric = dSubPopTimeFirstSymptoms;
					}
				}
			}
			break;
		case PopulationCharacteristics::timeFirstRemoved:
			dMetric = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
			for (int iSubPop = 0; iSubPop<PopulationCharacteristics::nPopulations; iSubPop++) {
				if (populations[iSubPop] != NULL) {
					double dSubPopTimeFirstRemoval = populations[iSubPop]->getEpidemiologicalMetric(iMetric);
					if (!bMetricDefined) {
						bMetricDefined = true;
						dMetric = dSubPopTimeFirstRemoval;
					}
					if (dSubPopTimeFirstRemoval < dMetric) {
						dMetric = dSubPopTimeFirstRemoval;
					}
				}
			}
			break;
		default:
			//Inoculum depositions/exports:
			dMetric = 1.0;
			for(int iSubPop=0; iSubPop<PopulationCharacteristics::nPopulations; iSubPop++) {
				if(populations[iSubPop] != NULL) {
					if(!bMetricDefined) {
						dMetric = 0.0;
					}
					dMetric +=  populations[iSubPop]->getEpidemiologicalMetric(iMetric);
				}
			}
			break;
		}

	}
	
	return dMetric;

}

int LocationMultiHost::getNumberOfEvents(int iPop) {
	
	if(populations[iPop] != NULL) {
		return populations[iPop]->getNumberOfEvents();
	} else {
		return 0;
	}
	
}

int LocationMultiHost::setEventIndex(int iPop, int iEvtIndex) {
	
	if(populations[iPop] != NULL) {
		populations[iPop]->setEventIndex(iEvtIndex);
		return 1;
	} else {
		return 0;
	}

}

int LocationMultiHost::getRatesAffectedByRanged(EventInfo lastEvent) {
	
	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL) {
			populations[i]->getRatesAffectedByRanged(lastEvent);
		}
	}
	
	return 1;
}

int LocationMultiHost::getRatesInf(int iPopulation) {

	if (iPopulation < 0) {
		for (int i = 0; i < nPopulations; i++) {
			if (populations[i] != NULL) {
				populations[i]->getRatesInf();
			}
		}
	} else {
		if (iPopulation < nPopulations) {
			if (populations[iPopulation] != NULL) {
				populations[iPopulation]->getRatesInf();
			}
		}
	}

	return 1;
}

int LocationMultiHost::getRatesFull() {
	
	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL) {
			populations[i]->getRatesFull();
		}
	}
	
	return 1;
}

int LocationMultiHost::haveEvent(int iPop, int iEvent, int nTimesToExecute, bool bExecuteEverything) {
	if (populations[iPop] != NULL) {
		return populations[iPop]->haveEvent(iEvent, nTimesToExecute, bExecuteEverything);
	}
	return 0;
}

double LocationMultiHost::dGetHostScaling(int iPop) {
	
	if(populations[iPop] != NULL) {
		return populations[iPop]->dGetHostScaling();
	}

	return 1.0;

}

int LocationMultiHost::trialInfection(int iFromPopType, double dX, double dY) {
	
	//Decide which population spore actually lands on...

	double dTotalArea = 1.0;
	if (!RateManager::bStandardSpores) {
		//With the magnetic spores, the spores are never allowed to land on empty space:
		dTotalArea = 0.0;
		for (int iPop = 0; iPop < nPopulations; iPop++) {
			if (populations[iPop] != NULL) {
				dTotalArea += populations[iPop]->getDensity();
			}
		}
	}

	double dSporeLocation = dUniformRandom()*dTotalArea;
	double dTotalCoverage = 0.0;
	for (int iPop = 0; iPop < nPopulations; iPop++) {
		if (populations[iPop] != NULL) {
			dTotalCoverage += populations[iPop]->getDensity();
			if (dTotalCoverage > dSporeLocation) {
				//Pass the trial to the selected population:
				return populations[iPop]->trialInfection(iFromPopType, dX, dY);
				//it landed on a susceptible from pop i, so the spore can't land on anything else
			}
		}
	}

	return 0;
}

//TODO: Generalised transitionHosts function move all|arbitrary classes to other classes
int LocationMultiHost::causeInfection(int nInfections, int iPop) {
	
	if(iPop < 0) {
		//Distribute infections over the separate populations using susceptibility weightings:
		double dTotalWeighting = 0.0;

		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				dTotalWeighting += populations[i]->getSusceptibility();
			}
		}

		if(dTotalWeighting > 0.0) {

			for(int i=0; i<nPopulations; i++) {
				if(populations[i] != NULL) {
					int nToCause = int(ceil(nInfections*populations[i]->getSusceptibility()/dTotalWeighting));
					nInfections -= nToCause;//TODO: this biases it to the first location...

					populations[i]->causeInfection(nToCause);
				}
			}

		} else {
			//Just try and stuff them anywhere possible then:
			for(int i=0; i<nPopulations; i++) {
				if(populations[i] != NULL) {
					return populations[i]->causeInfection(nInfections);
				}
			}
		}

		return 0;
	}


	if(populations[iPop] != NULL) {
		return populations[iPop]->causeInfection(nInfections);
	}
	
	return 0;
}

int LocationMultiHost::causeSymptomatic(int iPop, double dProportion) {
	
	if(iPop < 0) {
		
		//Just try and stuff them anywhere possible:
		for(int i=0; i<nPopulations; i++) {
			if(populations[i] != NULL) {
				return populations[i]->causeSymptomatic(dProportion);
			}
		}

		return 1;
		
	}

	if(populations[iPop] != NULL) {
		return populations[iPop]->causeSymptomatic(dProportion);
	}
	
	return 0;
}

int LocationMultiHost::causeNonSymptomatic(int iPop, double dProportion) {
	
	if(iPop < 0) {
		
		//Just try and stuff them anywhere possible:
		for(int i=0; i<nPopulations; i++) {
			int nTotal = 0;
			if(populations[i] != NULL) {
				nTotal += populations[i]->causeNonSymptomatic(dProportion);
			}
			return nTotal;
		}
		
	}

	if(populations[iPop] != NULL) {
		return populations[iPop]->causeNonSymptomatic(dProportion);
	}
	
	return 0;
}

int LocationMultiHost::causeCryptic(int iPop, double dProportion) {

	if (iPop < 0) {

		//Just try and stuff them anywhere possible:
		for (int i = 0; i<nPopulations; i++) {
			int nTotal = 0;
			if (populations[i] != NULL) {
				nTotal += populations[i]->causeCryptic(dProportion);
			}
			return nTotal;
		}

	}

	if (populations[iPop] != NULL) {
		return populations[iPop]->causeCryptic(dProportion);
	}

	return 0;
}

int LocationMultiHost::causeExposed(int iPop, double dProportion) {

	if (iPop < 0) {

		//Just try and stuff them anywhere possible:
		for (int i = 0; i<nPopulations; i++) {
			int nTotal = 0;
			if (populations[i] != NULL) {
				nTotal += populations[i]->causeExposed(dProportion);
			}
			return nTotal;
		}

	}

	if (populations[iPop] != NULL) {
		return populations[iPop]->causeExposed(dProportion);
	}

	return 0;
}

int LocationMultiHost::causeRemoved(int iPop, double dProportion) {
	
	if(iPop < 0) {
		
		//Just try and stuff them anywhere possible:
		for(int i=0; i<nPopulations; i++) {
			int nTotal = 0;
			if(populations[i] != NULL) {
				nTotal += populations[i]->causeRemoved(dProportion);
			}
			return nTotal;
		}
		
	}

	if(populations[iPop] != NULL) {
		return populations[iPop]->causeRemoved(dProportion);
	}
	
	return 0;
}



int LocationMultiHost::scrub(int bResubmitRates) {
	
	//DEBUG:
	//Landscape::debugCompareStateToDPCs();
	//DEBUG:

	int nPopsInfectedBefore = 0;

	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL) {
			if (populations[i]->getCurrentlyInfectedFraction() > 0.0) {
				nPopsInfectedBefore++;
			}
			populations[i]->scrub(bResubmitRates);
		}
	}
	
	int nPopsInfectedAfter = 0;
	for (int i = 0; i < nPopulations; i++) {
		if (populations[i] != NULL) {
			if (populations[i]->getCurrentlyInfectedFraction() > 0.0) {
				//Something else still infected, so don't report as the overall locations loss of infection:
				nPopsInfectedAfter++;
			}
		}
	}

	//See if it has lost infection in all populations:
	if (nPopsInfectedBefore > 0 && nPopsInfectedAfter == 0) {
		//Nothing now infected, so report as the location losing infection:
		SummaryDataManager::submitCurrentLossOfInfection(-1, xPos, yPos);
	}

	if (nPopsInfectedAfter > 0 && nPopsInfectedBefore == 0) {
		//Nothing now infected, so report as the location losing infection:
		SummaryDataManager::submitCurrentInfection(-1, xPos, yPos);
	}

	if (nPopsInfectedAfter > 0) {
		//Obviously the first time a location is ever infected:
		SummaryDataManager::submitFirstInfection(-1, xPos, yPos);
	}

	//DEBUG:
	//Landscape::debugCompareStateToDPCs();
	//DEBUG:

	return 1;
}

void LocationMultiHost::createDataStorage(int nElements) {

	for(int i=0; i<nPopulations; i++) {
		if(populations[i] != NULL) {
			populations[i]->createDataStorage(nElements);
		}
	}

	return;
}

double LocationMultiHost::getDataStorageElement(int iPop, int iElement) {
	if(populations[iPop] != NULL) {
		return populations[iPop]->getDataStorageElement(iElement);
	} else {
		return 0.0;
	}
}

void LocationMultiHost::setDataStorageElement(int iPop, int iElement, double dValue) {

	if(populations[iPop] != NULL) {
		return populations[iPop]->setDataStorageElement(iElement, dValue);
	}

	return;
}

void LocationMultiHost::incrementDataStorageElement(int iPop, int iElement, double dValue) {

	if(populations[iPop] != NULL) {
		return populations[iPop]->incrementDataStorageElement(iElement, dValue);
	}

	return;
}
