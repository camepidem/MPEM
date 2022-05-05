//Population.h

#ifndef POPULATION_H
#define POPULATION_H 1

#include "classes.h"
#include "InoculumTrap.h"

class Population {
	
public:

	//Constructor(s)/destructor:
	Population(LocationMultiHost* pParentLoc, int iPop, double dDense);
	~Population();
	
	//Initialisation:
	static int initPopulations(char *szFullModelString);
	static int destroyPopulations();

	static int setLandscape(Landscape *myWorld);
	static int setRateManager(RateManager *myRates);
	static int setDispersalManager(DispersalManager *myDispersal);

	//Array of characteristics of all different population types:
	static vector<PopulationCharacteristics *> pGlobalPopStats;

	int populate(double *pdHostAllocation = NULL);

	std::string debugGetStateString();

	double getDensity();

	double *getHostAllocation();

	//Get overall infectiousness of the population:
	double getInfectiousness();

	//Infectivity/Susceptibility
	int setInfBase(double dInf), setSusBase(double dSus);
	int scaleInfBase(double dInf), scaleSusBase(double dSus);
	int setInfSpray(double dInf), setSusSpray(double dSus);

	double getInfSpray(), getSusSpray();

	double getSusceptibility();

	//Detection and Control:
	//TODO: Move information about detection state of landscape to the detection intervention
	bool setSearch(bool bNewSearch);
	bool setKnown(bool bNewSearch);

	bool getSearch();
	bool getKnown();

	double getInfectiousFraction();
	double getDetectableFraction();

	double getCurrentlyInfectedFraction();

	int resetDetectionStatus();

	//Control inferface methods:
	double cull(double effectiveness, double dMaxAllowedHost);
	double rogue(double effectiveness, double dMaxAllowedHost);
	double sprayAS(double effectiveness, double dMaxAllowedHost), sprayPR(double effectiveness, double dMaxAllowedHost);

	//Statistics: Epidemiological Metrics:
	//Get general additional data: (type indexing stored as static public members in PopulationCharacteristics)
	double getEpidemiologicalMetric(int iMetric);

	//Evolution interface  methods:
	//Access number of events present in this population
	int getNumberOfEvents();

	int getRatesAffectedByRanged(EventInfo lastEvent);
	int getRatesFull();
	int getRatesInf();
	int haveEvent(int iEvent, int nTimesToExecute = 1, bool bExecuteEverything = false);

	//Sets start event index for population
	int setEventIndex(int iEvtIndex);

	//Virtual Sporulation Handling:
	//Returns the fraction of the cell occupied by this populations susceptibles:
	double getSusceptibleCoverage();
	
	//Handle a spore landing at location dX,dY in the population
	double dGetHostScaling();
	int trialInfection(int iFromPopType, double dX, double dY);

	//Force population to cause an infection
	int causeInfection(int nInfections);

	int causeSymptomatic(double dProportion = 1.0);
	int causeNonSymptomatic(double dProportion = 1.0);
	int causeCryptic(double dProportion = 1.0);
	int causeExposed(double dProportion = 1.0);
	int causeRemoved(double dProportion = 1.0);

	//Clean the population back to a blank starting state
	int scrub(int bResubmitRates);

	void createDataStorage(int nElements);
	double getDataStorageElement(int iElement);
	void setDataStorageElement(int iElement, double dValue);
	void incrementDataStorageElement(int iElement, double dValue);

	//Lists:
	vector<ListNode<Population>*> pLists;
	void joinList(int iList), leaveList(int iList);

	//Pointer to the parent location of this population:
	LocationMultiHost *pParentLocation;

protected:

	//Management structures:
	static Landscape* world;
	static RateManager* rates;
	static DispersalManager* dispersal;

	static int nPopulations;

	//Index of populations first event in the evolution structure. ie should submit rates as iEvolutionIndex + iLocalEventIndex
	int iEvolutionIndex;

	//Pointer to the characteristics of this population:
	PopulationCharacteristics *pPopStats;

	//Overall host density:
	double dDensity;

	//Numbers of hosts in each class:
	//Storage:
	//0 -- (nClasses-1)	:	Host class numbers
	//nClasses		:	(Optional) Total number ever removed
	vector<int> nHosts;

	//Total number of host units actually allocated:
	int nTotalHostUnits;
	//Initial data for numbers of hosts if necessary to store:
	vector<int> nInitialdata;

	//Quantities in special classes:
	int nInfectious;
	int nDetectable;

	//Calculate special classes:
	int calculateInfectious();
	int calculateDetectable();

	//Scale factor to account for the effect of having more than
	//the expected number of hosts in a location
	double dHostScaling;

	//Infectivity/Susceptibility
	//Current overall effective:
	double dInfectivity, dSusceptibility;
	//Base (eg average weather)
	double dInfBase, dSusBase;
	//Modified due to spray
	double dInfSpray, dSusSpray;

	//Recalculate Sus/Inf whever component values are changed
	int calculateSusceptibility(), calculateInfectivity();

	//Detection and control:
	bool bIsSearched, bIsKnown;

	//Epidemiological Metrics:
	vector<double> pdEpidemiologicalMetrics;

	//Inoculum tracking:
	vector<InoculumTrap> sporeDepositionFromSourcePopulation;
	InoculumTrap sporeProduction;

	//Data storage: Used by events like RiskMap to have locations store data
	//TODO: Terrible idea, how did this ever happen?
	vector<double> pdDataStorage;

	//Evolution methods:
	int setRateGeneral(int iEvent);//Not for S-> or virtual sporulation
	int setRateSLoss();
	int quickRescaleRateSLoss(double dAfterOverBefore);//Used when know factor by which rate has changed rather than recalculating from scratch
	int setRateVirtualSporulation();

	int calculateEffectiveSusceptibles();
	double calculateRatePrimaryInfection();

	int quickSetRateSLoss(EventInfo lastEvent);

	//Method to move hosts about: (handles all complexities behind scenes)
	double transitionHosts(int nToTransition, int iFromClass, int iToClass);

	//Internal cleaning function
	//Clean the population back to a blank starting state, but do not reset data such as accumulated spore deposition
	int scrub_(int bUseInitialState, int bResubmitRates);

};

#endif
