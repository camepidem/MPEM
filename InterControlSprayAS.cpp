//InterControlSprayAS.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "LocationMultiHost.h"
#include "ListManager.h"
#include "RasterHeader.h"
#include "PopulationCharacteristics.h"
#include "InterControlSprayAS.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterControlSprayAS::InterControlSprayAS() {
	timeSpent = 0;
	InterName = "ControlSprayAntisporulant";
	totalCost = 0.0;

	setCurrentWorkingSubection(InterName, CM_POLICY);

	enabled = 0;
	readValueToProperty("SprayASEnable",&enabled,-1, "[0,1]");

	if(enabled) {

		frequency = 1.0;
		readValueToProperty("SprayASFrequency",&frequency,2, ">0");

		if (frequency <= 0.0) {
			reportReadError("ERROR: SprayASFrequency (%f) <= 0.0. Must be positive.\n", frequency);
		}

		timeFirst = world->timeStart + frequency;
		readValueToProperty("SprayASFirst", &timeFirst, 2, ">=0");
		timeNext = timeFirst;

		if (timeFirst < world->timeStart) {
			printk("Warning: SprayASFirst (%f) was specified for before the simulation start time (%f). No controls will be applied before the start of the simulation.\n", timeFirst, world->timeStart);

			//Advance to first time greater than or equal to simulation start:
			int nPeriodsBehind = ceil((world->timeStart - timeFirst) / frequency);
			double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

			printk("Warning: Advancing SprayASFirst time to earliest time >= simulation start time by advancing start time %d periods to %f.\n", nPeriodsBehind, newTimeFirst);

			timeFirst = newTimeFirst;
		}

		radius = 0.0;
		readValueToProperty("SprayASRadius",&radius,2, ">-1");

		effect = 1.0;
		readValueToProperty("SprayASEffect",&effect,2, "[0,1]");

		cost = 1.0;
		readValueToProperty("SprayASCost",&cost,-2, ">=0");
		
		bOuputStatus = 0;
		readValueToProperty("SprayASOutputFinalState", &bOuputStatus, -2, "[0,1]");

		//TODO: Option to control on a per population basis?
		iPopulationToControl = -1;

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterControlSprayAS::~InterControlSprayAS() {
	//do nothing
}

int InterControlSprayAS::intervene() {
	//printf("SPRAYING!\n");

	//Spraying code
	//make list of all places that have been detected, then spray in radius about each
	int nToControl = 0;
	LocationMultiHost *activeLocation;
	ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;

	//Pass over all active locations
	while(pNode != NULL) {
		activeLocation = pNode->data;
		if(activeLocation->getKnown(iPopulationToControl)) {
			if(activeLocation->getInfSpray(iPopulationToControl) > effect) {
				nToControl++;
			}
		}
		pNode = pNode->pNext;
	}


	if(nToControl > 0) {

		vector<LocationMultiHost *> toControl(nToControl);

		pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;

		int iLocation = 0;
		//Pass over all active locations
		while(pNode != NULL) {
			activeLocation = pNode->data;
			if(activeLocation->getKnown(iPopulationToControl)) {
				if(activeLocation->getInfSpray(iPopulationToControl) > effect) {
					toControl[iLocation] = activeLocation;
					iLocation++;
				}
			}
			pNode = pNode->pNext;
		}

		for(int i = 0; i < nToControl; i++) {
			//for all locations within radius about toControl[i]
			double r2 = radius*radius;
			int centreX, centreY, startX, startY, endX, endY;

			centreX = toControl[i]->xPos;
			centreY = toControl[i]->yPos;

			startX = int(centreX-radius-1);
			startY = int(centreY-radius-1);

			endX = int(centreX+radius+1);
			endY = int(centreY+radius+1);

			startX = max(0,startX);
			startY = max(0,startY);

			endX = min(world->header->nCols,endX);
			endY = min(world->header->nRows,endY);

			for(int j=startX;j<endX;j++) {
				for(int k=startY;k<endY;k++) {
					int u = (j-centreX);
					u *= u;
					int v = (k-centreY);
					v *= v;
					if(double(u + v) <= r2) {
						double dist = sqrt(double(u+v));
						//cull in squares so that effectively cull by proportion of cell that is within radius
						double actualEffectiveness = 1.0 + radius - dist;
						actualEffectiveness = min(max(actualEffectiveness,0.0),1.0);
						actualEffectiveness *= effect;
						LocationMultiHost *tgtLoc = world->locationArray[j+world->header->nCols*k];
						if(tgtLoc != NULL) {
							if(tgtLoc->getInfSpray(iPopulationToControl) > actualEffectiveness) {
								totalCost += cost*tgtLoc->sprayAS(actualEffectiveness, 1.0, iPopulationToControl);
							}
						}
					}
				}
			}
		}

		//because the spraying has changed the landscape will need to 
		//reset last event information to force update of whole landscape after intervention
		world->bFlagNeedsInfectionRateRecalculation();

	}

	timeNext += frequency;

	return 1;
}

int InterControlSprayAS::finalAction() {
	//return area sprayed
	//printf("ControlSprayAS::finalAction()\n");

	if(enabled) {
		
		char fName[256];

		if(enabled && bOuputStatus) {
			for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
				sprintf(fName,"%s%s%d_SPRAYAS%s", szPrefixOutput(), world->szPrefixLandscapeRaster, i, world->szSuffixTextFile);
				writeRasterPopulationProperty(fName, i, &LocationMultiHost::getInfSpray);
			}
		}

	}

	return 1;
}

void InterControlSprayAS::writeSummaryData(FILE *pFile) {
	//Output area sprayed, no. trees culled(by densities, densities infectious, cost of culling
	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Cost %12.12f\n\n", totalCost);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
