//LocationMultiHost.h

//This object defines the properties and interface of an abstracted location with the rest of the landscape

#ifndef LOCATIONMULTIHOST_H
#define LOCATIONMULTIHOST_H 1

#include "classes.h"
#include <vector>

class LocationMultiHost {
	
public:

	//Constructor(s)/destructor:
	LocationMultiHost(int nX, int nY);
	~LocationMultiHost();

	//Initialization:
	static int setLandscape(Landscape *myWorld);

	//Initialise all statics and buffers needed:
	static int initLocations();

	std::string debugGetStateString();

	//Read the density rasters to populations
	int readDensities(double *pdDensities);
	double *getDensities();

	//Populate child populations with hosts. Will automatically create clean populations if no initial data is to be read.
	int populate(double **pdHostAllocations);
	double **getHostAllocations();

	//Get overall infectiousness of a population:
	double getInfectiousness(int iPop);

	//Infectivity/Susceptibility
	int setInfBase(double dInf, int iPop), setSusBase(double dSus, int iPop);
	int scaleInfBase(double dInf, int iPop), scaleSusBase(double dSus, int iPop);
	int setInfSpray(double dInf, int iPop), setSusSpray(double dSus, int iPop);
	double getInfSpray(int iPop), getSusSpray(int iPop);

	//Detection and Control:
	int setSearch(int bNewSearch, int iPop);
	int setKnown(int bNewKnown, int iPop);

	bool getSearch(int iPop);
	bool getKnown(int iPop);

	int resetDetectionStatus();

	double getCoverage(int iPop);
	double getSusceptibleCoverageProportion(int iPop);
	double getInfectiousCoverageProportion(int iPop);
	double getDetectableCoverageProportion(int iPop);

	double getSusceptibilityWeightedCoverage(int iPop);

	//Control inferface methods:
	double cull(double effectiveness, double dMaxAllowedHost, int iPop);
	double rogue(double effectiveness, double dMaxAllowedHost, int iPop);
	double sprayAS(double effectiveness, double dMaxAllowedHost, int iPop), sprayPR(double effectiveness, double dMaxAllowedHost, int iPop);

	//Lists:
	std::vector<ListNode<LocationMultiHost> *> pLists;

	//Handle sub populations leaving/joining lists:
	void subPopulationLeftList(int ListIndex);
	void subPopulationJoinedList(int ListIndex);

	void joinList(int iList), leaveList(int iList);

	void subPopulationFirstInfected(int iPop);
	void subPopulationInitiallyInfected(int iPop);

	//Tracking of current infection status:
	void subPopulationCurrentInfection(int iPop);
	void subPopulationLossOfCurrentInfection(int iPop);

	//Special cases for use during scrubbing of landscape:
	void subPopulationInitialLossOfCurrentInfection(int iPop);
	void subPopulationInitialGainOfCurrentInfection(int iPop);

	void subPopulationCurrentSusceptible(int iPop);
	void subPopulationLossOfCurrentSusceptible(int iPop);

	//Statistics: Epidemiological Metrics:
	//Get general additional data: (type indexing stored as static public members in PopulationCharacteristics)
	double getEpidemiologicalMetric(int iPop, int iMetric);

	//Evolution interface  methods:
	//Access number of events present in this location for a given sub-population:
	//(0 if population not present)
	int getNumberOfEvents(int iPop);

	//Sets start event index for sub populations
	//Returns number of events in the location
	int setEventIndex(int iPop, int iEvtIndex);

	//Calculation of rates:
	int getRatesAffectedByRanged(EventInfo lastEvent);
	int getRatesInf(int iPopulation = -1);
	int getRatesFull();

	//Execution of events:
	int haveEvent(int iPop, int iEvent, int nTimesToExecute = 1, bool bExecuteEverything = false);

	//Handle a spore landing at location dX,dY in the location
	//return -1 for failiure, else population index
	double dGetHostScaling(int iPop);
	int trialInfection(int iFromPopType, double dX, double dY);

	//Force location to cause an infection
	int causeInfection(int nInfections, int iPop);

	int causeSymptomatic(int iPop, double dProportion = 1.0);
	int causeNonSymptomatic(int iPop, double dProportion = 1.0);
	int causeCryptic(int iPop, double dProportion = 1.0);
	int causeExposed(int iPop, double dProportion = 1.0);
	int causeRemoved(int iPop, double dProportion = 1.0);

	//Clean the location back to a blank starting state
	int scrub(int bResubmitRates);

	static LinkedList<LocationMultiHost> **pLocationLists;

	void createDataStorage(int nElements);
	double getDataStorageElement(int iPop, int iElement);
	void setDataStorageElement(int iPop, int iElement, double dValue);
	void incrementDataStorageElement(int iPop, int iElement, double dValue);

	//Physical Location:
	int xPos, yPos;

	//Inoculum transport:
	//TODO:

	//Spore influx:
	//Need to store by source population

	//Spore efflux:
	//Need to store by population (and kernel?)

	//Used to flag location as having been acted upon
	int actionFlag;

protected:
	
	//Management structures:
	static Landscape *world;
	
	//Total number of separate populations
	static int nPopulations;

	//The locations child Populations:
	std::vector<Population *> populations;

	//Buffers:
	static std::vector<double> pdDensityBuffer;
	static std::vector<double *> pdHostAllocationsBuffer;

};

#endif
