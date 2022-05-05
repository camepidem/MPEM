//PopulationCharacteristics.h

#ifndef POPULATIONCHARACTERISTICS_H
#define POPULATIONCHARACTERISTICS_H 1

#include "classes.h"

//TODO: Export most stuff to abstracted model class to allow for more complex models w/arbitrary graphs and formalise treatment of different types of events
class PopulationCharacteristics {
	
public:

	//Constructor(s)/destructor:
	PopulationCharacteristics(char *szModelStr, int iPop);
	~PopulationCharacteristics();

	//Total number of separate populations
	static int nPopulations;
	static const int ALL_POPULATIONS;

	//Count of active populations of this type:
	int nActiveCount;

	//Index of this population:
	int iPopulation;

	//Number of distinct host classes:
	int nClasses;

	int iSusceptibleClass;
	int iLastClass;
	int iNEverRemoved;

	//Total Number of class quantities tracked (might include nEverRemoved)
	int nTotalClasses;

	//ASCII representation of model:
	char *szModelString;

	//Maximum number of hosts to use to represent a location of this type:
	static int nMaxHosts;

	//Array of names of each class
	vector<char *> pszClassNames;

	//Identifier of the kernel for this population
	int iKernel;

	//Number of events that can occur. Ordinarily either (nPopulations - 1) + nClasses or nClasses + 1 depending on if hosts can become susceptible again
	//ie full cycle of host individual + virtual sporulation
	//Indexing goes as 
	//Event	0...nPopulations - 1:	Infection of susceptible by spore from population i
	//Event nPopulations:	Primary Infection
	//... every other host transition ...
	//(Potentially a rebirth transition)
	//Event nEvents-1:	Virtual Sporulation

	int nEvents;

	int iInfectionEventsStart;
	int iInfectionEventsEnd;
	
	int iPrimaryInfectionEvent;

	int iStandardEventsStart;
	int iStandardEventsEnd;
	
	int iVirtualSporulationEvent;

	//Indicates if the last class has rebirth:
	bool bLastClassHasRebirth;

	//Indicates if keeping count of total number ever removed:
	bool bTrackingNEverRemoved;

	//Class of removed hosts. If -ve then have no removed class and 
	//any hosts removed by controls will be placed in the S class
	int iRemovedClass;

	//Array indicating if each host classes are infectious/detectable
	vector<bool> pbClassIsInfectious;
	vector<bool> pbClassIsDetectable;

	//Epidemiological Metrics:
	//TODO: Now that we live in the future, and have much richer enums, this should probably be one
	static const int severity					= 0;
	static const int timeFirstInfection			= 1;
	static const int timeFirstSymptomatic		= 2;
	static const int timeFirstRemoved			= 3;

	static const int nEpidemiologicalMetricStatic = 4;

	static int inoculumExport;
	static int inoculumDeposition;

	static int nEpidemiologicalMetrics;

	static vector<string> vsEpidemiologicalMetricsNames;

	//Lists: //TODO: Have this not be constructed in ListManager() ...
	LinkedList<Population> **pPopulationLists;

	//Buffers:
	vector<double> pdHostAllocationBuffer;

protected:


};

#endif
