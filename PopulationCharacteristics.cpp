//PopulationCharacteristics.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "PopulationCharacteristics.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

vector<string> PopulationCharacteristics::vsEpidemiologicalMetricsNames;
int PopulationCharacteristics::nEpidemiologicalMetrics;
int PopulationCharacteristics::inoculumExport;
int PopulationCharacteristics::inoculumDeposition;
int PopulationCharacteristics::nPopulations;
int PopulationCharacteristics::nMaxHosts;
const int PopulationCharacteristics::ALL_POPULATIONS = -1;

//Constructor(s):
PopulationCharacteristics::PopulationCharacteristics(char *szModelStr, int iPop) {

	iPopulation = iPop;

	//Start count at zero:
	nActiveCount = 0;

	//Parse model string to compartmental model:
	//Store the ASCII model representation:
	szModelString = szModelStr;

	//Default to using first kernel:
	iKernel = 0;

	char szString[256];
	sprintf(szString,"Population_%d_Kernel",iPop);

	if(bIsKeyMode()) {
		printToKeyFile(" Population_~i[0,#nPopulations#)~_Kernel* 0 ~i[0,#nKernels#)~");

		//Need to define some of these for other bits of keyfile generation to work:
		//As if simplest possible SI model:
		nEvents = 3;

		iInfectionEventsStart = 0;
		iInfectionEventsEnd = 1;

		iPrimaryInfectionEvent = 1;

	} else {
		readValueToProperty(szString,&iKernel,-1);

		//Establish number of classes:
		int iStr = 0;
		while(szModelString[iStr] != '\0') {
			iStr++;
		}


		bLastClassHasRebirth = false;
		bTrackingNEverRemoved = false;

		iRemovedClass = -1;
		iNEverRemoved = -1;

		//Check if last class gets put back into S:
		if(szModelString[iStr-1] == 'S') {
			//Have rebirth, so last character was a duplicate:
			nClasses = iStr - 1;
			bLastClassHasRebirth = true;
			//Establish if have removals and need to track nEverRemoved:
			if(szModelString[nClasses - 1] == 'R') {
				bTrackingNEverRemoved = true;
				iNEverRemoved = nClasses;
			}
		} else {
			nClasses = iStr;
		}

		iSusceptibleClass = 0;
		iLastClass = nClasses - 1;

		//Events:
		nEvents = 0;

		//Infection events from each source population:
		int nInfectionEvents = nPopulations + 1;

		iInfectionEventsStart = nEvents;
		iInfectionEventsEnd = iInfectionEventsStart + nInfectionEvents - 1;
		
		//Primary infection event:
		iPrimaryInfectionEvent = iInfectionEventsEnd;
		
		nEvents += nInfectionEvents;


		//Standard transition events for all classes except first class (infection event) and last class (end of the line):
		int nStandardEvents = nClasses - 2;

		//Possible transition event for last class back to S:
		if(bLastClassHasRebirth) {
			++nStandardEvents;
		}

		iStandardEventsStart = nEvents;
		iStandardEventsEnd = iStandardEventsStart + nStandardEvents - 1;

		nEvents += nStandardEvents;

		
		//Virtual sporulation event:
		iVirtualSporulationEvent = nEvents;

		++nEvents;


		//Classes:
		nTotalClasses = nClasses;

		if(bTrackingNEverRemoved) {
			nTotalClasses++;
		}

		//Sanity check:
		//Check there are no duplications of classes in the string:
		for(int i=0; i<nClasses; i++) {//Want to not flag S at the end...
			for(int j=i+1; j<nClasses; j++) {
				if(szModelString[i] == szModelString[j]) {
					reportReadError("ERROR: Found the same class (%c) twice in model: %s\n", szModelString[i], szModelString);
				}
			}
		}
		//End sanity check

		//Record the properties of each class
		pszClassNames.resize(nTotalClasses);
		pbClassIsInfectious.resize(nClasses);
		pbClassIsDetectable.resize(nClasses);


		//Check first class is S:
		char cCurrentClass = szModelString[0];
		if(cCurrentClass != 'S') {
			reportReadError("ERROR: Illegal model-First class specified is not susceptible\n       First class read: %c\n",cCurrentClass);
		}
		pbClassIsDetectable[0] = false;
		pbClassIsInfectious[0] = false;

		pszClassNames[0] = "SUSCEPTIBLE";

		for(int i=1; i<nClasses; i++) {

			cCurrentClass = szModelString[i];

			switch(cCurrentClass) {
			case 'S':
				//ERROR: already had S
				reportReadError("ERROR: Illegal model- Has more than one susceptible class: %s\n", szModelString);
				break;
			case 'E':
				pszClassNames[i] = "EXPOSED";

				pbClassIsDetectable[i] = false;
				pbClassIsInfectious[i] = false;
				break;
			case 'C':
				pszClassNames[i] = "CRYPTIC";

				pbClassIsDetectable[i] = false;
				pbClassIsInfectious[i] = true;
				break;
			case 'D':
				pszClassNames[i] = "DETECTABLE";

				pbClassIsDetectable[i] = true;
				pbClassIsInfectious[i] = false;
				break;
			case 'I':
				pszClassNames[i] = "INFECTIOUS";

				pbClassIsDetectable[i] = true;
				pbClassIsInfectious[i] = true;
				break;
			case 'R':
				iRemovedClass = i;

				pszClassNames[i] = "REMOVED";

				pbClassIsDetectable[i] = false;
				pbClassIsInfectious[i] = false;
				break;
			default:
				//Error: Unknown class ...
				reportReadError("ERROR: Unknown model class: %c in model %s\n", cCurrentClass, szModelString);
				break;
			}

		}

		//Handle the nEverRemoved, yet again
		if(bTrackingNEverRemoved) {
			pszClassNames[nTotalClasses-1] = "EVER_REMOVED";
		}

		//Buffers:
		pdHostAllocationBuffer.resize(nTotalClasses);

	}

}

//Destructor:
PopulationCharacteristics::~PopulationCharacteristics() {

	if(szModelString != NULL) {
		delete [] szModelString;
		szModelString = NULL;
	}

}
