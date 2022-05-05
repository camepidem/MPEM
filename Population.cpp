//Population.cpp

#include "stdafx.h"
#include "PopulationCharacteristics.h"
#include "LocationMultiHost.h"
#include "Landscape.h"
#include "ListManager.h"
#include "SummaryDataManager.h"
#include "DispersalManager.h"
#include "DispersalKernel.h"
#include "RateManager.h"
#include "myRand.h"
#include "RasterHeader.h"
#include "InoculumTrap.h"
#include "cfgutils.h"
#include "Population.h"

Landscape* Population::world;
RateManager* Population::rates;
DispersalManager* Population::dispersal;
int Population::nPopulations;
vector<PopulationCharacteristics *> Population::pGlobalPopStats;

//Constructor(s):
Population::Population(LocationMultiHost* pParentLoc, int iPop, double dDense) {

	//Link population to its statistics:
	pPopStats = pGlobalPopStats[iPop];

	//Report location will be populated:
	pPopStats->nActiveCount++;

	//Assign population its parent Location:
	pParentLocation = pParentLoc;

	dDensity = dDense;

	//Lists:
	pLists.resize(ListManager::nLists);

	for(int iList = 0; iList < ListManager::nLists; ++iList) {
		pLists[iList] = NULL;
	}

	//Infectivity/Susceptibility:
	dInfBase = 1.0;
	dSusBase = 1.0;
	dInfSpray = 1.0;
	dSusSpray = 1.0;

	calculateInfectivity();
	calculateSusceptibility();

	//Detection/Control:
	bIsSearched = true;
	bIsKnown = false;

	//Epidemiological Metrics:
	//Allocate and initialise to null values:
	pdEpidemiologicalMetrics.resize(PopulationCharacteristics::nEpidemiologicalMetrics);

	pdEpidemiologicalMetrics[PopulationCharacteristics::severity] = 0.0;
	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstInfection] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstSymptomatic] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstRemoved] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values

	//Inoculum Import:
	if (world->bTrackingInoculumDeposition()) {
		sporeDepositionFromSourcePopulation.resize(nPopulations);
		for (int iSourcePopulation = 0; iSourcePopulation < nPopulations; iSourcePopulation++) {
			sporeDepositionFromSourcePopulation[iSourcePopulation] = InoculumTrap(iPop, iSourcePopulation, world->timeStart);
		}
	}

	//Inoculum Export:
	if (world->bTrackingInoculumExport()) {
		sporeProduction = InoculumTrap(iPop, iPop, world->timeStart);
	}

}

//Destructor:
Population::~Population() {

}

int Population::initPopulations(char *szFullModelString) {

	//Parse szFullModelString to separate hosts

	int maxHosts = 1;
	readValueToProperty("MaxHosts", &maxHosts, 1, ">0");

	PopulationCharacteristics::nMaxHosts = maxHosts;

	nPopulations = 1;

	char cModelDelimeter = '-';
	int iString = 0;
	while(szFullModelString[iString] != '\0') {
		if(szFullModelString[iString] == cModelDelimeter) {
			nPopulations++;
		}
		iString++;
	}

	pGlobalPopStats.resize(nPopulations);

	//TODO: Need a proper init for this
	PopulationCharacteristics::nPopulations = nPopulations;

	//Epidemiological metrics:
	PopulationCharacteristics::vsEpidemiologicalMetricsNames.resize(PopulationCharacteristics::nEpidemiologicalMetricStatic);

	PopulationCharacteristics::vsEpidemiologicalMetricsNames[PopulationCharacteristics::severity] = string("SEVERITY");
	PopulationCharacteristics::vsEpidemiologicalMetricsNames[PopulationCharacteristics::timeFirstInfection] = string("TIMEFIRSTINFECTION");
	PopulationCharacteristics::vsEpidemiologicalMetricsNames[PopulationCharacteristics::timeFirstSymptomatic] = string("TIMEFIRSTSYMPTOMATIC");
	PopulationCharacteristics::vsEpidemiologicalMetricsNames[PopulationCharacteristics::timeFirstRemoved] = string("TIMEFIRSTREMOVED");

	if (world->bTrackingInoculumExport()) {
		PopulationCharacteristics::inoculumExport = PopulationCharacteristics::vsEpidemiologicalMetricsNames.size();
		PopulationCharacteristics::vsEpidemiologicalMetricsNames.push_back(string("INOCULUM_EXPORT"));
	}

	if (world->bTrackingInoculumDeposition()) {
		PopulationCharacteristics::inoculumDeposition = PopulationCharacteristics::vsEpidemiologicalMetricsNames.size();
		for (int iPop = 0; iPop < nPopulations; iPop++) {
			PopulationCharacteristics::vsEpidemiologicalMetricsNames.push_back(string("INOCULUM_DEPOSITION_FROM_") + static_cast<ostringstream*>(&(ostringstream() << iPop))->str());
		}
	}

	PopulationCharacteristics::nEpidemiologicalMetrics = PopulationCharacteristics::vsEpidemiologicalMetricsNames.size();

	//Create PopulationCharacteristics for each host
	iString = 0;
	for(int i=0; i<nPopulations; i++) {

		char *szModelString = new char[1024];

		//Copy each sub model to a string to pass for conversion to model characteristics:
		int jString = 0;
		while(szFullModelString[iString] != '\0' && szFullModelString[iString] != cModelDelimeter) {
			szModelString[jString] = szFullModelString[iString];
			iString++;
			jString++;
		}

		//Move beyond last character read/placed:
		iString++;

		szModelString[jString] = '\0';

		pGlobalPopStats[i] = new PopulationCharacteristics(szModelString,i);

		//Don't delete szModelString as it will be kept by PopulationCharacteristics
		//TODO: ^ is horrible
	}

	return nPopulations;

}

int Population::destroyPopulations() {

	if(pGlobalPopStats.size() != 0) {
		for(int i=0; i<nPopulations; i++) {
			delete pGlobalPopStats[i];
		}
	}

	return 1;
}

int Population::setLandscape(Landscape *myWorld) {

	world = myWorld;

	return 1;
}

int Population::setRateManager(RateManager *myRates) {

	rates = myRates;

	return 1;
}

int Population::setDispersalManager(DispersalManager *myDispersal) {

	dispersal = myDispersal;

	return 1;
}

int Population::populate(double *pdHostAllocation) {

	int nClasses = pPopStats->nClasses;

	//Allocate storage for host numbers (certainly all classes):
	int nData = nClasses;
	if(pPopStats->bTrackingNEverRemoved) {
		//Optionally might be tracking the total number ever removed if we lost some to rebirths (or whatever)
		nData++;
	}

	nHosts.resize(nData);

	if(pPopStats->bTrackingNEverRemoved) {
		//Start off with no removals:
		nHosts[pPopStats->iNEverRemoved] = 0;
	}

	//Expected number of hosts to create:
	double dHostsExpected = dDensity*PopulationCharacteristics::nMaxHosts;

	//If host allocation null then create clean population:
	if(pdHostAllocation != NULL) {

		//Sanity Check:
		double dTotal = 0.0;
		for(int i=0; i<nClasses; i++) {
			double dVal = pdHostAllocation[i];
			if(dVal < 0.0 || dVal > 1.0) {
				//Error: out of range!
				reportReadError("ERROR: At location x=%d, y=%d\nIn population %d\nHost class %s has fraction=%f: out of range [0,1]\n",pParentLocation->xPos,pParentLocation->yPos,pPopStats->iPopulation,pPopStats->pszClassNames[i],dVal);
			}
			dTotal += dVal;
		}

		if(abs(dTotal - 1.0) > 0.01) {
			reportReadError("ERROR: At location x=%d, y=%d\nIn population %d\nTotal host fraction allocation=%f differs too much from 1.0\nFull allocation:\n",pParentLocation->xPos,pParentLocation->yPos,pPopStats->iPopulation,dTotal);
			for (int i = 0; i<nClasses; i++) {
				double dVal = pdHostAllocation[i];
				reportReadError("%s = %f\n", pPopStats->pszClassNames[i], dVal);
			}
		}
		//End Sanity check

		//Total number
		nTotalHostUnits = 0;
		//Allocate individual classes:
		for(int iClass = 0; iClass < nClasses; ++iClass) {

			//In a continuous world:
			double dExactAllocation = dDensity*pdHostAllocation[iClass] * PopulationCharacteristics::nMaxHosts;

			//Be generous and round up:
			double dTargetAllocation = ceil(dExactAllocation);

			//but not too generous, otherwise floating point errors introduce too many extra hosts
			if (dTargetAllocation - dExactAllocation > 1e-3) {
				dTargetAllocation--;
			}

			//and make sure that every class that had a positive allocation gets something at least, even if the previous step would eliminate it:
			if (dTargetAllocation == 0.0 && dExactAllocation > 0.0) {
				dTargetAllocation++;
			}

			//Allocate integral quantity of hosts to the class:
			nHosts[iClass] = int(dTargetAllocation);

			//Increment total 
			nTotalHostUnits += nHosts[iClass];

		}

	} else {

		//Total is ceiling of expected number
		nTotalHostUnits = int(ceil(dHostsExpected));

		//Put all of them in the susceptibles class:
		nHosts[pPopStats->iSusceptibleClass] = nTotalHostUnits;
		for(int iClass = 0; iClass < nClasses; ++iClass) {
			if(iClass != pPopStats->iSusceptibleClass) {
				//Allocate class:
				nHosts[iClass] = 0;
			}
		}

	}

	//Initial data:
	//If need to store data, and actually have something interesting to store:
	if(world->iInitialDataRequired == Landscape::iInitialDataSave && nHosts[pPopStats->iSusceptibleClass] != nTotalHostUnits) {
		//Allocate an array and store the intial status of the population:
		//N.B. We don't just want a copy of the state vector, as that contains things beyond the initial state
		nInitialdata.resize(nClasses);
		for(int i=0; i<nClasses; i++) {
			nInitialdata[i] = nHosts[i];
		}
	}

	//(Might) have overallocated a bit:
	dHostScaling = dHostsExpected/double(nTotalHostUnits);

	if (dHostScaling < 1e-2) {
		cout << "SUPER LOW HOST SCALING AT x=" << pParentLocation->xPos << " y=" << pParentLocation->yPos << " pop=" << pPopStats->iPopulation << " scaling=" << dHostScaling << endl;
	}

	//Set the host scaling:
	//host scaling is now explicitly included in calculateInfectivity
	//Deprecated: scaleInfBase(dHostScaling);
	calculateInfectivity();

	//Special variables:
	nInfectious = calculateInfectious();
	nDetectable = calculateDetectable();


	//Epidemiological Metrics:
	pdEpidemiologicalMetrics[PopulationCharacteristics::severity] = getCurrentlyInfectedFraction();

	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstInfection] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	if(pdEpidemiologicalMetrics[PopulationCharacteristics::severity] > 0.0) {
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstInfection] = world->timeStart;
		pParentLocation->subPopulationInitiallyInfected(pPopStats->iPopulation);//This special case function will do both the first infection and current infection - need to use this because not all populations are initialised yet
	}

	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstSymptomatic] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	if (nDetectable > 0) {
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstSymptomatic] = world->timeStart;
	}

	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstRemoved] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	if (pPopStats->iRemovedClass > 0 && nHosts[pPopStats->iRemovedClass] > 0) {
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstRemoved] = world->timeStart;
	}

	if (nHosts[pPopStats->iSusceptibleClass] > 0) {
		pParentLocation->subPopulationCurrentSusceptible(pPopStats->iPopulation);
	}

	//Summary Data:
	for(int i=0; i<nClasses; i++) {
		//Submit all host data, summary data will be zero, so sumbit current host quantity. (Don't need to include nRem as this must start at zero anyway)
		SummaryDataManager::submitHostChange(pPopStats->iPopulation, i, dHostScaling*nHosts[i]/double(PopulationCharacteristics::nMaxHosts), pParentLocation->xPos, pParentLocation->yPos);
	}

	//Manage lists:
	if(nHosts[pPopStats->iSusceptibleClass] > 0) {
		//Have susceptibles, so join list
		joinList(ListManager::iSusceptibleList);
	}

	if(nInfectious > 0) {
		joinList(ListManager::iInfectiousList);
	}

	return 1;

}

std::string Population::debugGetStateString()
{
	std::stringstream os;

	for (int iClass = 0; iClass < pPopStats->nClasses; ++iClass) {
		os << pPopStats->pszClassNames[iClass] << "=" << nHosts[iClass] << std::endl;
	}

	return os.str();
}

double Population::getDensity() {
	return dDensity;
}

double *Population::getHostAllocation() {

	int nClasses = pPopStats->nClasses;
	double dTotal = double(nTotalHostUnits);
	for(int i=0; i<nClasses; i++) {
		pPopStats->pdHostAllocationBuffer[i] = nHosts[i]/dTotal;
	}

	return &pPopStats->pdHostAllocationBuffer[0];
}

double Population::getInfectiousness() {
	return dInfectivity*nInfectious;
}

int Population::setInfBase(double dInf) {
	dInfBase = dInf;
	return calculateInfectivity();
}

int Population::setSusBase(double dSus) {
	dSusBase = dSus;
	return calculateSusceptibility();
}

int Population::scaleInfBase(double dInf) {
	dInfBase *= dInf;
	return calculateInfectivity();
}

int Population::scaleSusBase(double dSus) {
	dSusBase *= dSus;
	return calculateSusceptibility();
}

int Population::setInfSpray(double dInf) {
	dInfSpray = dInf;
	return calculateInfectivity();
}

int Population::setSusSpray(double dSus) {
	dSusSpray = dSus;
	return calculateSusceptibility();
}

double Population::getInfSpray() {
	return dInfSpray;
}

double Population::getSusSpray() {
	return dSusSpray;
}

double Population::getSusceptibility() {
	return dSusceptibility;
}

bool Population::setSearch(bool bNewSearch) {
	return bIsSearched = bNewSearch;
}

bool Population::setKnown(bool bNewKnown) {

	bIsKnown = bNewKnown;

	return bIsKnown;
}

bool Population::getSearch() {
	return bIsSearched;
}

bool Population::getKnown() {
	return bIsKnown;
}

int Population::resetDetectionStatus() {

	//TODO: Figure out what to do here...

	return 0;
}

double Population::cull(double effectiveness, double dMaxAllowedHost) {

	//Establish if population has living hosts:
	int nLiving = 0;
	int iRemovedClass = pPopStats->iRemovedClass;
	if(iRemovedClass < 0) {//If there is no removed class, put destroyed hosts into the S class //TODO: is this desireable behaviour? 
		iRemovedClass = 0;
	}
	int nClasses = pPopStats->nClasses;
	for(int i=0; i<nClasses; i++) {
		if(i != iRemovedClass) {
			nLiving += nHosts[i];
		}
	}

	double dTotalCost = 0.0;

	if(nLiving > 0) {
		//Something to deal with:
		double totalHost = (nLiving / double(PopulationCharacteristics::nMaxHosts)) * dGetHostScaling();

		if (totalHost * effectiveness > dMaxAllowedHost) {
			effectiveness = dMaxAllowedHost / totalHost;//Only control, on average, up to what we are allowed to affect
		}

		//Want to remove each living host with probability effectiveness, unless we are constrained by dMaxAllowedHost
		//in which case we want to have removed up to (or just over) that amount, and distributed randomly over all the living classes
		for (int iClass = 0; iClass < nClasses; ++iClass) {
			if (iClass != iRemovedClass && nHosts[iClass] > 0) {
				int nToLose = nSlowBinomial(nHosts[iClass], effectiveness);

				//Handles all consequences of moving hosts about:
				dTotalCost += transitionHosts(nToLose, iClass, iRemovedClass);
			}
		}

		if(dTotalCost > 0.0) {
			//Handle touched list
			joinList(ListManager::iTouchedList);
		}

	}

	return dTotalCost;

}

double Population::rogue(double effectiveness, double dMaxAllowedHost) {

	//Establish if population has a removed class:
	int nClasses = pPopStats->nClasses;
	int iRemovedClass = pPopStats->iRemovedClass;
	if(iRemovedClass < 0) {
		iRemovedClass = 0;
	}

	if(nDetectable > 0) {
		//Something to deal with:

		double dTotalCost = 0.0;
		double totalHost = (nDetectable / double(PopulationCharacteristics::nMaxHosts)) * dGetHostScaling();

		if (totalHost * effectiveness > dMaxAllowedHost) {
			effectiveness = dMaxAllowedHost / totalHost;//Only control, on average, up to what we are allowed to affect
		}
		for(int i=0; i<nClasses; i++) {

			//If class is symptomatic:
			if(pPopStats->pbClassIsDetectable[i]) {
				int nToLose = nSlowBinomial(nHosts[i],effectiveness);

				//Handles all consequences of moving hosts about:
				dTotalCost += transitionHosts(nToLose, i, iRemovedClass);
			}
		}

		if(dTotalCost > 0.0) {
			//Handle touched list
			joinList(ListManager::iTouchedList);
		}

		return dTotalCost;

	} else {
		//had nothing symptomatic, so nothing to rogue
		return 0.0;
	}

}

double Population::sprayAS(double effectiveness, double dMaxAllowedHost) {
	setInfSpray(1.0 - effectiveness*dMaxAllowedHost);//Not entirely correct, as we have already baked in a constraint on what area is affected into the effectiveness parameter, but a reasonable approximation in almost all circumstances.
	return dDensity*dMaxAllowedHost;
}

double Population::sprayPR(double effectiveness, double dMaxAllowedHost) {
	setSusSpray(1.0 - effectiveness*dMaxAllowedHost);
	return dDensity*dMaxAllowedHost;
}

double Population::getEpidemiologicalMetric(int iMetric) {
	if(iMetric < PopulationCharacteristics::nEpidemiologicalMetricStatic) {
		return pdEpidemiologicalMetrics[iMetric];
	} else {
		if(iMetric == PopulationCharacteristics::inoculumExport) {
			return sporeProduction.getCurrentAccumulation(world->time, rates);
		} else {
			int iSourcePop = iMetric - PopulationCharacteristics::inoculumDeposition;
			return sporeDepositionFromSourcePopulation[iSourcePop].getCurrentAccumulation(world->time, rates);
		}
	}
}

int Population::getNumberOfEvents() {
	return pPopStats->nEvents;
}

int Population::getRatesAffectedByRanged(EventInfo lastEvent) {

	if (quickSetRateSLoss(lastEvent)) {
		joinList(ListManager::iTouchedList);
	}

	return 1;
}

int Population::getRatesFull() {

	int nPositive = 0;

	//Infection rate: S->
	nPositive += setRateSLoss();

	//Generic rates:
	for(int iEvent = pPopStats->iStandardEventsStart; iEvent <= pPopStats->iStandardEventsEnd; ++iEvent) {
		nPositive += setRateGeneral(iEvent);
	}

	//VirtualSporulation
	nPositive += setRateVirtualSporulation();

	//Touched
	if(nPositive > 0) {
		joinList(ListManager::iTouchedList);
	}

	return nPositive;
}

int Population::getRatesInf() {

	int nPositive = setRateSLoss();

	nPositive += setRateVirtualSporulation();

	return nPositive;
}

int Population::haveEvent(int iEvent, int nTimesToExecute, bool bExecuteEverything) {

	if(iEvent == pPopStats->iVirtualSporulationEvent) {
		//Virtual Sporulation
		if(nInfectious > 0) {
			//Choice of starting location within cell:
			double xOffset = 0.5;
			double yOffset = 0.5;

			for (int iSporulation = 0; iSporulation < nTimesToExecute; ++iSporulation) {
				DispersalManager::kernelLongTrial(pPopStats->iPopulation, pParentLocation->xPos + xOffset, pParentLocation->yPos + yOffset);
			}

			return 1;
		} else {
			return 0;
		}
	}

	//Otherwise event is standard transition:
	//Default to an infection event, primary or otherwise:
	int iClassToMove = 0;
	//Otherwise, see if it is something else:
	if(iEvent >= pPopStats->iStandardEventsStart && iEvent <= pPopStats->iStandardEventsEnd) {
		iClassToMove = iEvent - (pPopStats->iStandardEventsStart - 1);
	}

	//Check legality:
	int nMovableHosts = nHosts[iClassToMove];
	if(nMovableHosts >= 1) {

		int transitionSuccessful;

		if (nMovableHosts < nTimesToExecute) {
			nTimesToExecute = nMovableHosts;
		}

		if (bExecuteEverything) {
			nTimesToExecute = nMovableHosts;
		}

		//Is a host to move out:
		int iClassToMoveTo = iClassToMove + 1;
		if (iClassToMove >= pPopStats->iLastClass) {
			iClassToMoveTo = pPopStats->iSusceptibleClass;
		}
		transitionSuccessful = (transitionHosts(nTimesToExecute, iClassToMove, iClassToMoveTo) > 0.0);

		return transitionSuccessful;

	} else {
		//Illegal event:
		return bExecuteEverything;
	}

}

int Population::setEventIndex(int iEvtIndex) {
	return iEvolutionIndex = iEvtIndex;
}

double Population::getSusceptibleCoverage() {

	double dCurrentSusCoverage = 0.0;
	if (nHosts.size() > 0) {//Little hack to check for first time initialisation being complete without rewriting everything to be initialised more cleanly now we live in the future and have much much ore ram...
		dCurrentSusCoverage = dHostScaling*double(nHosts[pPopStats->iSusceptibleClass]) / double(PopulationCharacteristics::nMaxHosts);
	}

	return dCurrentSusCoverage;
}

double Population::dGetHostScaling() {
	return dHostScaling;
}

int Population::trialInfection(int iFromPopType, double dX, double dY) {

	//Virtual spore has landed here
	double dCanonicalSporeScaling = DispersalManager::dGetVSPatchScaling(pPopStats->iPopulation);

	//Log landing:
	if (world->bTrackingInoculumDeposition()) {
		sporeDepositionFromSourcePopulation[iFromPopType].virtualSpore(dCanonicalSporeScaling / double(PopulationCharacteristics::nMaxHosts));
	}

	//and see if it should cause an infection
	//Virtual spore has landed on a host, see if it was a susceptible and see if the spore successfully infects:
	double dSusceptibleFraction = getSusceptibleCoverage()/getDensity();

	if (!rates->bStandardSpores) {
		if (dSusceptibleFraction > 0.0) {
			dSusceptibleFraction = 1.0;
		} else {
			dSusceptibleFraction = 0.0;
		}
	}

	if(dUniformRandom() < dSusceptibleFraction*dSusceptibility*dCanonicalSporeScaling/dHostScaling) {
		//Should cause infection:
		haveEvent(iFromPopType);
		return 1;
	}

	return 0;
}

int Population::causeInfection(int nInfections) {

	int bCausedInfectiousness = 0;
	int nMaxPossibleInfections = min(nHosts[pPopStats->iSusceptibleClass],nInfections);

	//Infect individuals until they first become infectious:
	//TODO: What happens if have class that does not become infectious?
	//eg US SOD Oaks ... Need to consider scope of legal models...
	//Might need to refine this to just cause an infection rather than infectiousness

	int iClass = 0;
	while (!pPopStats->pbClassIsInfectious[iClass] && iClass < pPopStats->nClasses) {
		int iEvent = pPopStats->iPrimaryInfectionEvent;
		if (iClass > 0) {
			//New event indexing has all infections events at the front...
			iEvent = pPopStats->iStandardEventsStart + iClass - 1;
		}
		haveEvent(iEvent, nMaxPossibleInfections);
		iClass++;
	}
	if (pPopStats->pbClassIsInfectious[iClass]) {
		bCausedInfectiousness = 1;
	}


	return nMaxPossibleInfections;
}

int Population::causeSymptomatic(double dProportion) {

	//Push all asymptomatic hosts towards symptomatic classes:
	int nChanges = 0;
	for (int iClass = 0; !pPopStats->pbClassIsDetectable[iClass] && iClass < pPopStats->nClasses; iClass++) {//For all the asymptomatic classes
		
		int iEvent = pPopStats->iPrimaryInfectionEvent;
		if (iClass > 0) {
			//New event indexing has all infections events at the front...
			iEvent = pPopStats->iStandardEventsStart + iClass - 1;
		}
		
		int nAttempts = int(ceil(double(nHosts[iClass]) * dProportion));
		nChanges += haveEvent(iEvent, nAttempts);//Shuffle them along one

	}

	return nChanges;
}

int Population::causeCryptic(double dProportion) {

	int iCrypticClass = 0;


	for (int iClass = 0; iClass < pPopStats->nClasses; iClass++) {//For all the asymptomatic classes
		if (!pPopStats->pbClassIsDetectable[iClass] && pPopStats->pbClassIsInfectious[iClass]) {
			iCrypticClass = iClass;
			break;
		}
	}

	//Push all asymptomatic hosts towards symptomatic classes:
	int nChanges = 0;
	for (int iClass = 0; iClass < iCrypticClass; iClass++) {//For all the asymptomatic classes

		int iEvent = pPopStats->iPrimaryInfectionEvent;
		if (iClass > 0) {
			//New event indexing has all infections events at the front...
			iEvent = pPopStats->iStandardEventsStart + iClass - 1;
		}

		int nAttempts = int(ceil(double(nHosts[iClass]) * dProportion));
		nChanges += haveEvent(iEvent, nAttempts);//Shuffle them along one
	}

	return nChanges;
}

int Population::causeExposed(double dProportion) {

	int nAttempts = int(ceil(double(nHosts[pPopStats->iSusceptibleClass]) * dProportion));
	return haveEvent(pPopStats->iPrimaryInfectionEvent, nAttempts);//Shuffle them along one

}

int Population::causeRemoved(double dProportion) {

	if(pPopStats->iRemovedClass < 0) {
		//No removed class, so can't do anything:
		printk("Warning: No removed class, so unable to remove hosts\n");
		return 0;
	}

	int nChanged = 0;
	//Push all non susceptible asymptomatic hosts off the end and back on to the start:
	for (int iClass = 0; iClass < pPopStats->nClasses; iClass++) {
		if (iClass != pPopStats->iRemovedClass) {
			int nAttempts = int(ceil(double(nHosts[iClass]) * dProportion));
			nChanged += (transitionHosts(nAttempts, iClass, pPopStats->iRemovedClass) > 0.0);
		}
	}

	return nChanged;
}

int Population::causeNonSymptomatic(double dProportion) {

	//Manually return population to non symptomatic state

	int nChanged = 0;
	//Push all non susceptible hosts back to the susceptible state:
	for (int iClass = 0; iClass < pPopStats->nClasses; iClass++) {
		if (iClass != pPopStats->iSusceptibleClass) {
			int nAttempts = int(ceil(double(nHosts[iClass]) * dProportion));
			nChanged += (transitionHosts(nAttempts, iClass, pPopStats->iSusceptibleClass) > 0.0);
		}
	}

	return nChanged;
}

int Population::scrub_(int bUseInitialState, int bResubmitRates) {

	//TODO: Is this the proper form for scrub?
	//		Need to remove the special initial data setting?

	int nClasses = pPopStats->nClasses;

	double dPriorInfectedFraction = getCurrentlyInfectedFraction();
	int nPriorSusceptibles = nHosts[pPopStats->iSusceptibleClass];

	//Clean up host distribution:
	//Submit all changes in host data. (Don't need to include nRem as this must start at zero anyway)
	vector<int> targetState;

	if(nInitialdata.size() != 0 && bUseInitialState) {
		
		//Have specific starting state:
		targetState = nInitialdata;

	} else {

		//If have no special initial data:
		targetState.resize(nClasses);

		//Move everything to susceptible:
		targetState[pPopStats->iSusceptibleClass] = nTotalHostUnits;

		//Zero everything else:
		for(int i=0; i<nClasses; i++) {
			if(i != pPopStats->iSusceptibleClass) {
				targetState[i] = 0;
			}
		}

	}

	int nClassesChanged = 0;

	//Adopt target state:
	for (int i = 0; i < nClasses; i++) {
		int deltaHostUnits = targetState[i] - nHosts[i];
		SummaryDataManager::submitHostChange(pPopStats->iPopulation, i, dHostScaling*deltaHostUnits / double(PopulationCharacteristics::nMaxHosts), pParentLocation->xPos, pParentLocation->yPos);
		if (deltaHostUnits != 0) {
			nClassesChanged++;
		}
		nHosts[i] = targetState[i];
	}

	//Handle nEverRemoved if it exists:
	if(pPopStats->bTrackingNEverRemoved) {
		nHosts[pPopStats->iNEverRemoved] = 0;
	}

	nInfectious = calculateInfectious();
	nDetectable = calculateDetectable();

	//Reset spray status:
	setInfSpray(1.0);
	setSusSpray(1.0);

	//Manage status reporting:
	if(nHosts[pPopStats->iSusceptibleClass] > 0) {
		//Have susceptibles, so join list unless already there from a previous run:
		joinList(ListManager::iSusceptibleList);
	} else {
		//Have no susceptibles, so want to leave list unless already on it:
		leaveList(ListManager::iSusceptibleList);
	}

	if(nInfectious > 0) {
		joinList(ListManager::iInfectiousList);
	} else {
		leaveList(ListManager::iInfectiousList);
	}

	//If population is interesting:
	bool bInInitialState = true;
	if(nInitialdata.size() != 0) {
		for(int iClass = 0; iClass < nClasses; ++iClass) {
			if(nHosts[iClass] != nInitialdata[iClass]) {
				bInInitialState = false;
				break;
			}
		}
	}

	//TODO: Why are we particularly interested in not all susceptible rather than just not in initial state?
	if(nHosts[pPopStats->iSusceptibleClass] != nTotalHostUnits || bInInitialState == false) {
		joinList(ListManager::iTouchedList);
	} else {
		leaveList(ListManager::iTouchedList);
	}

	double dNewInfectedFraction = getCurrentlyInfectedFraction();

	//Was infected and no longer infected:
	if(dPriorInfectedFraction > 0.0 && dNewInfectedFraction <= 0.0) {
		pParentLocation->subPopulationInitialLossOfCurrentInfection(pPopStats->iPopulation);
	}

	//Now infected and previously not infected:
	if(dNewInfectedFraction > 0.0 && dPriorInfectedFraction <= 0.0) {
		pParentLocation->subPopulationInitialGainOfCurrentInfection(pPopStats->iPopulation);
	}

	//Lost all susceptibles:
	if (nPriorSusceptibles > 0 && nHosts[pPopStats->iSusceptibleClass] <= 0) {
		pParentLocation->subPopulationLossOfCurrentSusceptible(pPopStats->iPopulation);
	}

	//Have gone from not susceptible to susceptible:
	if (nHosts[pPopStats->iSusceptibleClass] > 0 && nPriorSusceptibles <= 0) {
		pParentLocation->subPopulationCurrentSusceptible(pPopStats->iPopulation);
	}

	//Manage rates
	//In new structure, will always want to resubmit rates
	//May sometimes want to submit_dumb instead though
	for (int i = 0; i < pPopStats->nEvents; i++) {
		double dRate = 0.0;
		if (i == pPopStats->iPrimaryInfectionEvent) {
			dRate = calculateRatePrimaryInfection();
		}
		if (bResubmitRates) {
			rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, i, dRate);
		} else {
			rates->submitRate_dumb(pPopStats->iPopulation, iEvolutionIndex, i, dRate);
		}
	}

	//Obviously the rates are wrong here if the landscape is not totally clean,
	//but when this the case the world will flag if there needs to be a full recalculation
	//TODO: Horrible, horrible unenforced contract here... should presumably flag from inside here, or submit the correct rates...

	return nClassesChanged;

}

int Population::scrub(int bResubmitRates) {

	//Reset spore accumulation logging:
	if (world->bTrackingInoculumDeposition()) {
		for (int iSourcePop = 0; iSourcePop < nPopulations; iSourcePop++) {
			sporeDepositionFromSourcePopulation[iSourcePop].reset();
		}
	}

	if (world->bTrackingInoculumExport()) {
		sporeProduction.reset();
	}

	int bScrubResult = scrub_(1, bResubmitRates);

	//Manage Epidemiological Metrics:
	pdEpidemiologicalMetrics[PopulationCharacteristics::severity] = getCurrentlyInfectedFraction();
	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstInfection] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	if(pdEpidemiologicalMetrics[PopulationCharacteristics::severity] > 0.0) {
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstInfection] = world->timeStart;
	}

	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstSymptomatic] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	if (nDetectable > 0) {
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstSymptomatic] = world->timeStart;
	}

	pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstRemoved] = min(-1.0, world->timeStart - 1.0);//TODO: Coordinate magic undefined values
	if (pPopStats->iRemovedClass > 0 && nHosts[pPopStats->iRemovedClass] > 0) {
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstRemoved] = world->timeStart;
	}

	return bScrubResult;

}

void Population::createDataStorage(int nElements) {

	pdDataStorage.resize(nElements);

	//Initialise values to zero:
	for(int i = 0; i < nElements; i++) {
		pdDataStorage[i] = 0.0;
	}

	return;
}

double Population::getDataStorageElement(int iElement) {
	return pdDataStorage[iElement];
}

void Population::setDataStorageElement(int iElement, double dValue) {

	pdDataStorage[iElement] = dValue;

	return;
}

void Population::incrementDataStorageElement(int iElement, double dValue) {

	pdDataStorage[iElement] += dValue;

	return;
}

int Population::calculateInfectious() {

	int nClasses = pPopStats->nClasses;

	nInfectious = 0;
	for(int i=0; i<nClasses; i++) {
		if(pPopStats->pbClassIsInfectious[i]) {
			nInfectious += nHosts[i];
		}
	}

	return nInfectious;
}

int Population::calculateDetectable() {

	int nClasses = pPopStats->nClasses;

	nDetectable = 0;
	for(int i=0; i<nClasses; i++) {
		if(pPopStats->pbClassIsDetectable[i]) {
			nDetectable += nHosts[i];
		}
	}

	return nDetectable;
}

int Population::calculateInfectivity() {

	dInfectivity = dHostScaling*dInfBase*dInfSpray;

	return 1;
}

int Population::calculateSusceptibility() {

	dSusceptibility = dSusBase*dSusSpray;

	return 1;
}

double Population::getInfectiousFraction() {
	return double(nInfectious)/double(nTotalHostUnits);
}

double Population::getDetectableFraction() {
	return double(nDetectable)/double(nTotalHostUnits);
}

double Population::getCurrentlyInfectedFraction() {
	int nLivingNotHealthy = nTotalHostUnits - nHosts[pPopStats->iSusceptibleClass];
	//Discount removed if they exist:
	if(pPopStats->iRemovedClass > 0) {
		nLivingNotHealthy -= nHosts[pPopStats->iRemovedClass];
	}
	return double(nLivingNotHealthy)/double(nTotalHostUnits);
}

int Population::setRateGeneral(int iEvent) {

	//Just a normal transition rate
	int iClassOfEvent = iEvent - pPopStats->iStandardEventsStart + 1;
	if(rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, iEvent, (double)nHosts[iClassOfEvent]) > 0.0) {
		return 1;
	}

	return 0;

}

int Population::setRateVirtualSporulation() {

	double dSporeProductionRate = getInfectiousness();

	if (world->bTrackingInoculumExport()) {
		sporeProduction.reportRate(world->time, dSporeProductionRate, rates);
	}

	//Just like General, but with nInfectious rather than any specific host class:
	if(rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, pPopStats->iVirtualSporulationEvent, dSporeProductionRate) > 0.0) {
		return 1;
	}

	return 0;
}

int Population::calculateEffectiveSusceptibles() {

	int susFactor = nHosts[pPopStats->iSusceptibleClass];
	if (!rates->bStandardSpores) {
		if (susFactor > 0) {
			susFactor = PopulationCharacteristics::nMaxHosts;
		} else {
			susFactor = 0;
		}
	}

	return susFactor;
}

double Population::calculateRatePrimaryInfection() {
	return calculateEffectiveSusceptibles()*dSusceptibility;
}

int Population::setRateSLoss() {

	int bRateChanged = 0;

	int susFactor = calculateEffectiveSusceptibles();

	//Primary infection:
	double dRate = calculateRatePrimaryInfection();

	double dOldRate = rates->getRate(pPopStats->iPopulation, iEvolutionIndex, pPopStats->iPrimaryInfectionEvent);

	if (dRate != dOldRate) {
		//Rate has changed (probably...)
		rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, pPopStats->iPrimaryInfectionEvent, dRate);
		bRateChanged = 1;
	}

	//Secondary infection:
	int W = world->header->nCols;
	int H = world->header->nRows;
	int xPos = pParentLocation->xPos;
	int yPos = pParentLocation->yPos;
	DispersalKernel *kernel;
	for(int iSourcePop=0; iSourcePop<nPopulations; iSourcePop++) {

		double fSum = 0.0;

		//Can no longer optimise out calculation when have no susceptibles as have to log all deposition for output if that is enabled
		if (nHosts[pPopStats->iSusceptibleClass] > 0 || world->bTrackingInoculumDeposition()) {

			kernel = DispersalManager::getKernel(iSourcePop);
			int nK = kernel->getShortRange();

			int xMin = max(0, xPos - nK);
			int xMax = min(xPos + nK + 1, W);
			int yMin = max(0, yPos - nK);
			int yMax = min(yPos + nK + 1, H);

			int nCellsLocalArea = (xMax - xMin)*(yMax - yMin);//NB: don't want any +1's in here, as they are already incorporated into the x/yMax terms, as we go xMin < x <= xMax

			int nInfectiousCells = ListManager::locationLists[ListManager::iInfectiousList]->nLength;

			if (nCellsLocalArea < nInfectiousCells) {

				//There are more locations infected than within reach of this location
				int H0 = yMin*W;
				for (int y = yMin; y < yMax; y++) {
					for (int x = xMin; x < xMax; x++) {
						int nLoc = H0 + x;
						//Sums up contribution from each infected location weighted by dispersal kernel
						LocationMultiHost* loc = world->locationArray[nLoc];
						if (loc != NULL) {
							//Make sure to pick element such that it represents the deposition FROM location [dx, dy] TO this position: [xPos, yPos]
							fSum += kernel->KernelShort(xPos - x, yPos - y)*loc->getInfectiousness(iSourcePop);
						}
					}
					H0 += W;
				}

			} else {

				//There a smaller number of infectious cells than there are cells in the kernel:
				ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iInfectiousList]->pFirst->pNext;

				//TODO: Could do control by flagging all places to remove, then cluster analysis and then remove by cluster w/ piority per cluster...

				//Pass over all active locations
				while (pNode != NULL) {
					LocationMultiHost *loc = pNode->data;
					//Make sure to pick element such that it represents the deposition FROM location [dx, dy] TO this position: [xPos, yPos]
					fSum += kernel->KernelShort(xPos - loc->xPos, yPos - loc->yPos)*loc->getInfectiousness(iSourcePop);
					pNode = pNode->pNext;
				}

			}

		}

		double dRateInflux = fSum;

		double dRate = susFactor*dRateInflux*dSusceptibility;

		if (world->bTrackingInoculumDeposition()) {
			sporeDepositionFromSourcePopulation[iSourcePop].reportRate(world->time, dRateInflux*dDensity, rates);
		}

		double dOldRate = rates->getRate(pPopStats->iPopulation, iEvolutionIndex, iSourcePop);

		if(dRate != dOldRate) {
			//Rate has changed (probably...)
			rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, iSourcePop, dRate);
			bRateChanged = 1;
		}

	}

	return bRateChanged;

}

int Population::quickSetRateSLoss(EventInfo lastEvent) {

	//return setRateSLoss();

	double dRate = 0.0;

	//Can no longer optimise out calculation when have no susceptibles as have to log all deposition for output
	if(nHosts[pPopStats->iSusceptibleClass] > 0 || world->bTrackingInoculumDeposition()) {

		//The only rate that has changed is the rate of infection by the lastEventPop

		//Get the old rate:
		dRate = rates->getRate(pPopStats->iPopulation, iEvolutionIndex, lastEvent.iPop);

		//Get kernel value of deposition from LastEventLoc onto this Loc
		double dK = DispersalManager::getKernel(lastEvent.iPop)->KernelShort(pParentLocation->xPos - lastEvent.pLoc->xPos, pParentLocation->yPos - lastEvent.pLoc->yPos);

		//subtract lastEventLocs' previous contribution
		double df = lastEvent.dDeltaInfectiousness;

		//Change in flux is change in spores * fraction that reach here 
		double dFlux = df*dK;

		//weight with interacting classes, specific susceptibility etc
		int susFactor = nHosts[pPopStats->iSusceptibleClass];
		if (!rates->bStandardSpores) {
			if (susFactor > 0) {
				susFactor = PopulationCharacteristics::nMaxHosts;
			} else {
				susFactor = 0;
			}
		}

		dRate += dFlux*susFactor*dSusceptibility;

		if (dRate < 0.0) {
			dRate = 0.0;
		}

		rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, lastEvent.iPop, dRate);

		if (world->bTrackingInoculumDeposition()) {
			sporeDepositionFromSourcePopulation[lastEvent.iPop].modifyRate(world->time, dFlux, rates);
		}

		return 1;

	}

	//Don't neet to submit anything as the rate will already be zero with no susceptibles

	return 0;

}

int Population::quickRescaleRateSLoss(double dAfterOverBefore) {

	if (!rates->bStandardSpores) {
		if (dAfterOverBefore > 0.0) {
			dAfterOverBefore = 1.0;
		}
	}

	//Primary infection:
	double dRate = rates->getRate(pPopStats->iPopulation, iEvolutionIndex, pPopStats->iPrimaryInfectionEvent);

	//Submit the new rescaled rate:
	rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, pPopStats->iPrimaryInfectionEvent, dRate*dAfterOverBefore);


	//Secondary infection:
	//For each sporulation source:
	for(int iPop=0; iPop<nPopulations; iPop++) {

		//Get the old rate:
		double dRate = rates->getRate(pPopStats->iPopulation, iEvolutionIndex, iPop);

		//Submit the new rescaled rate:
		rates->submitRate(pPopStats->iPopulation, iEvolutionIndex, iPop, dRate*dAfterOverBefore);

		//No need to adjust the spore deposition rate, as that logs all spores falling on this host type, not just susceptibles

	}

	return 1;
}

double Population::transitionHosts(int nToTransition, int iFromClass, int iToClass) {

	//DEBUG::
	//Landscape::debugCompareStateToDPCs();
	//DEBUG::

	if (nToTransition == 0) {
		return 0.0;
	}

	//Take snapshots for comparison to manage lists & ranged events:
	int nSusceptibleBefore = nHosts[pPopStats->iSusceptibleClass];
	int nInfectiousBefore = nInfectious;
	int nDetectableBefore = nDetectable;

	double dPriorInfectedFraction = getCurrentlyInfectedFraction();
	double dPriorInfectiousness = getInfectiousness();

	//Report General Event Statistics:
	EventInfo thisEvent;
	thisEvent.pLoc = pParentLocation;
	thisEvent.iPop = pPopStats->iPopulation;
	thisEvent.bRanged = 0;

	//Actually move the hosts:
	nHosts[iFromClass] -= nToTransition;
	nHosts[iToClass] += nToTransition;

	//Manage special class data:
	if(pPopStats->pbClassIsInfectious[iFromClass]) {
		nInfectious -= nToTransition;
	}
	if(pPopStats->pbClassIsDetectable[iFromClass]) {
		nDetectable -= nToTransition;
	}

	if(pPopStats->pbClassIsInfectious[iToClass]) {
		nInfectious += nToTransition;
	}
	if(pPopStats->pbClassIsDetectable[iToClass]) {
		nDetectable += nToTransition;
	}

	//Global Summary Data:
	double dQuantityMoved = dHostScaling*nToTransition/double(PopulationCharacteristics::nMaxHosts);
	SummaryDataManager::submitHostChange(pPopStats->iPopulation, iFromClass, -dQuantityMoved, pParentLocation->xPos, pParentLocation->yPos);
	SummaryDataManager::submitHostChange(pPopStats->iPopulation, iToClass,	  dQuantityMoved, pParentLocation->xPos, pParentLocation->yPos);

	//If something went to removed and we need to track the total that have ever gone there:
	if(pPopStats->bTrackingNEverRemoved && iToClass == pPopStats->iRemovedClass) {
		nHosts[pPopStats->iNEverRemoved] += nToTransition;
		//As summary data manager does not explicitly track total numbers of hosts in each population, the nEverRemoved class is nClasses
		SummaryDataManager::submitHostChange(pPopStats->iPopulation, pPopStats->nClasses, dQuantityMoved, pParentLocation->xPos, pParentLocation->yPos);
	}

	if(nHosts[pPopStats->iSusceptibleClass] != nSusceptibleBefore) {
		//Change to number of susceptibles
		//S List
		if(nSusceptibleBefore == 0) {
			joinList(ListManager::iSusceptibleList);
		} else {
			if(nHosts[pPopStats->iSusceptibleClass] == 0) {
				leaveList(ListManager::iSusceptibleList);
			}
		}
	}

	if(nInfectious != nInfectiousBefore) {
		//It was ranged
		thisEvent.bRanged = 1;

		//Change the rate of virtual sporulation here:
		setRateVirtualSporulation();

		//Infectious List
		if(nInfectiousBefore == 0) {
			joinList(ListManager::iInfectiousList);
		} else {
			if(nInfectious == 0) {
				leaveList(ListManager::iInfectiousList);
			}
		}

	}

	//Manage Epidemiological Metrics
	double dCurrentInfectedFraction = getCurrentlyInfectedFraction();
	if(dCurrentInfectedFraction > pdEpidemiologicalMetrics[PopulationCharacteristics::severity]) {
		pdEpidemiologicalMetrics[PopulationCharacteristics::severity] = dCurrentInfectedFraction;
		//Can only be the first time this has been infected if it is the most infected it has been, so put this in here:
		if(pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstInfection] == min(-1.0, world->timeStart - 1.0)) {//TODO: Coordinate magic undefined values
			pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstInfection] = world->time;
			pParentLocation->subPopulationFirstInfected(pPopStats->iPopulation);
		}
	}

	if (nDetectable > 0 && pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstSymptomatic] == min(-1.0, world->timeStart - 1.0)) {//TODO: Coordinate magic undefined values
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstSymptomatic] = world->time;
	}

	if (pPopStats->iRemovedClass > 0 && nHosts[pPopStats->iRemovedClass] > 0 && pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstRemoved] == min(-1.0, world->timeStart - 1.0)) {//TODO: Coordinate magic undefined values
		pdEpidemiologicalMetrics[PopulationCharacteristics::timeFirstRemoved] = world->time;
	}

	//Lost all infection:
	if(dPriorInfectedFraction > 0.0 && dCurrentInfectedFraction <= 0.0) {
		pParentLocation->subPopulationLossOfCurrentInfection(pPopStats->iPopulation);
	}

	//Have gone from uninfected to infected:
	if(dCurrentInfectedFraction > 0.0 && dPriorInfectedFraction <= 0.0) {
		pParentLocation->subPopulationCurrentInfection(pPopStats->iPopulation);
	}

	//Lost all susceptibles:
	if (nSusceptibleBefore > 0 && nHosts[pPopStats->iSusceptibleClass] <= 0) {
		pParentLocation->subPopulationLossOfCurrentSusceptible(pPopStats->iPopulation);
	}

	//Have gone from not susceptible to susceptible:
	if (nHosts[pPopStats->iSusceptibleClass] > 0 && nSusceptibleBefore <= 0) {
		pParentLocation->subPopulationCurrentSusceptible(pPopStats->iPopulation);
	}

	//Manage Rates:
	vector<int> vTransitionClasses(2);
	vTransitionClasses[0] = iFromClass;
	vTransitionClasses[1] = iToClass;

	for(size_t iTransition = 0; iTransition < vTransitionClasses.size(); ++iTransition) {

		int iTransitionClass = vTransitionClasses[iTransition];

		//Special case for rate S-> as it can be costly to calculate
		if(iTransitionClass == pPopStats->iSusceptibleClass) {
			
			if(nSusceptibleBefore > 0) {
				//We can do the quick way:
				quickRescaleRateSLoss(double(nHosts[pPopStats->iSusceptibleClass])/double(nSusceptibleBefore));
			} else {
				//Need to calculate the rate from scratch. This cannot be done here.
				//Consider the case where in ScriptedInfection multiple locations are altered simultaneously.
				//If an infectious location L1 changes in infectiousness, 
				//and then a coupled location L2 recalculates its rateSLoss from scratch
				//The change in infectiousness of L1 will be applied twice, 
				//as the new infectiousness is already baked in to the rateInfection of L2, 
				//but then the reported change in infectiousness of L1 will be applied to its coupled region later on
				//causing the change to be double counted
				//Solution is to wait to calculate the rateSLoss until after all the reported deltas will be applied.
				//This applies no performance penalty, however anything necessitating an essay of this length indicates
				//a serious architectural defficiency, and so for this any many other reasons, 
				//should move to locations dispersing rather than populations pulling
				EventInfo birthEvent = thisEvent;//Only needs the pop and loc members
				world->recalculationQueue.push_back(birthEvent);
			}

		} else {

			//Only update this rate as a general rate if the class actually goes somewhere:
			//Update UNLESS ( IS LAST CLASS AND LAST CLASS DOES NOTHING )
			if(!(iTransitionClass == pPopStats->iLastClass && !pPopStats->bLastClassHasRebirth)) {
				int iTransitionClassEvent = (iTransitionClass - 1) + pPopStats->iStandardEventsStart;
				setRateGeneral(iTransitionClassEvent);
			}

		}

	}


	thisEvent.dDeltaInfectiousness = getInfectiousness() - dPriorInfectiousness;

	if (thisEvent.bRanged) {
		world->lastEvents.push_back(thisEvent);//Only now has the 'ranged' property been set
	}

	//Certainly have had something happen:
	joinList(ListManager::iTouchedList);

	//DEBUG::
	//Landscape::debugCompareStateToDPCs();
	//DEBUG::

	return dQuantityMoved;
}

void Population::joinList(int iList) {

	if(pLists[iList] == NULL) {
		pLists[iList] = pPopStats->pPopulationLists[iList]->addAsFirst(this);
		pParentLocation->subPopulationJoinedList(iList);
	}

	return;
}

void Population::leaveList(int iList) {

	if(pLists[iList] != NULL) {
		pLists[iList] = pPopStats->pPopulationLists[iList]->excise(pLists[iList]);
		pParentLocation->subPopulationLeftList(iList);
	}

	return;
}
