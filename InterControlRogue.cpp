//InterControlRogue

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "RasterHeader.h"
#include "ListManager.h"
#include "InterControlRogue.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterControlRogue::InterControlRogue() {
	timeSpent = 0;
	InterName = "ControlRogue";
	totalCost = 0.0;

	setCurrentWorkingSubection(InterName, CM_POLICY);

	enabled = 0;
	readValueToProperty("RogueEnable",&enabled,-1, "[0,1]");

	if(enabled) {

		frequency = 1.0;
		readValueToProperty("RogueFrequency",&frequency,2, ">0");

		if (frequency <= 0.0) {
			reportReadError("ERROR: RogueFrequency (%f) <= 0.0. Must be positive.\n", frequency);
		}

		timeFirst = world->timeStart + frequency;
		readValueToProperty("RogueFirst", &timeFirst, 2, "");
		timeNext = timeFirst;

		if (timeFirst < world->timeStart) {
			printk("Warning: RogueFirst (%f) was specified for before the simulation start time (%f). No controls will be applied before the start of the simulation.\n", timeFirst, world->timeStart);

			//Advance to first time greater than or equal to simulation start:
			int nPeriodsBehind = ceil((world->timeStart - timeFirst) / frequency);
			double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

			printk("Warning: Advancing RogueFirst time to earliest time >= simulation start time by advancing start time %d periods to %f.\n", nPeriodsBehind, newTimeFirst);

			timeFirst = newTimeFirst;
		}

		radius = 0.0;
		readValueToProperty("RogueRadius",&radius,2, ">-1");

		effectiveness = 1.0;
		readValueToProperty("RogueEffectiveness",&effectiveness,-2, "[0,1]");
		
		if(effectiveness > 1.0 || effectiveness < 0.0) {
			reportReadError("ERROR: RogueEffectiveness must be in range 0.0 to 1.0, read: %f\n",effectiveness);
		}

		cost = 1.0;
		readValueToProperty("RogueCost",&cost,-2, ">=0");

		//TODO: Option to control on a per population basis?
		iPopulationToControl = -1;

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterControlRogue::~InterControlRogue() {
	//do nothing
}

int InterControlRogue::intervene() {
	//printf("ROGUING!\n");

	//Roguing code:
	//Go through each place in Landscape, add to list of places to rogue about:
	int nToControl = 0;
	LocationMultiHost *activeLocation;
	ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;

	//Pass over all active locations
	while(pNode != NULL) {
		activeLocation = pNode->data;
		if(activeLocation->getKnown(iPopulationToControl)) {
			if(activeLocation->getDetectableCoverageProportion(iPopulationToControl) > 0.0) {
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
				if(activeLocation->getDetectableCoverageProportion(iPopulationToControl) > 0.0) {
					toControl[iLocation] = activeLocation;
					iLocation++;
				}
			}
			pNode = pNode->pNext;
		}

		for(int i=0;i<nToControl;i++) {
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
						//cull in squares so that effectively rogue by proportion of cell that is within radius
						double actualEffectiveness = 1.0 + radius - dist;
						actualEffectiveness = min(max(actualEffectiveness,0.0),1.0);
						actualEffectiveness *= effectiveness;
						LocationMultiHost *tgtLoc = world->locationArray[j+world->header->nCols*k];
						if(tgtLoc != NULL) {
							totalCost += cost*tgtLoc->rogue(actualEffectiveness,1.0, iPopulationToControl);
						}
					}
				}
			}
		}
		
		//because the roguing has changed the landscape will need to 
		//reset last event information to force update of whole landscape after intervention
		world->bFlagNeedsFullRecalculation();

	}

	timeNext += frequency;

	return 1;
}

int InterControlRogue::finalAction() {
	//return area rogued, no. trees rogued(by densities, densities infectious, cost of culling
	//printf("ControlRogue::finalAction()\n");

	return 1;
}

void InterControlRogue::writeSummaryData(FILE *pFile) {
	//Output area rogued, no. trees culled(by densities, densities infectious, cost of culling
	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Cost %12.12f\n\n", totalCost);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
