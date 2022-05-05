//InterHurricane.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "SummaryDataManager.h"
#include "ListManager.h"
#include "InterHurricane.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterHurricane::InterHurricane() {
	timeSpent = 0;
	InterName = "Hurricane";

	setCurrentWorkingSubection(InterName, CM_APPLICATION);

	enabled = 0;
	readValueToProperty("HurricaneEnable",&enabled,-1, "[0,1]");

	if(enabled) {
		
		printk("Warning: Hurricane Intervention is currently disabled pending a review of the hurricane model.\n");

		frequency = 1.0;
		readValueToProperty("HurricaneFrequency",&frequency,2, ">0");

		timeFirst = world->timeStart + frequency;
		readValueToProperty("HurricaneFirst",&timeFirst,2, ">=0");
		timeNext = timeFirst;

		severity = 1.0;
		readValueToProperty("HurricaneSeverity",&severity,2, ">0");

		enabled = 0;
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterHurricane::~InterHurricane() {

}

int InterHurricane::intervene() {
	//printf("HURRICANE!\n");

	//Hurricane will affect the landscape by throwing out (severity*existing infectious locations) of new challenges
	//over the landscape

	//number of challenges to make:
	int nChallenges = 0;// int(severity*SummaryDataManager::getHostListNumbers(-1)[ListManager::iInfectiousList]);

	//?? probably not... nChallenges = min(nChallenges,world->nLAct);

	//pick locations
	//vector<int> nLStart(nChallenges);

	//for(int i = 0; i<nChallenges; i++) {
	//	nLStart[i] = int(dUniformRandom()*world->nLAct);
	//}

	////Sort the locations into an order
	//qsort(nLStart,nChallenges,sizeof(int),compare);

	////Go through active list to find each location in turn:
	//Location *activeLocation;
	//activeLocation = world->Anchor->pActNext;

	//int currentLocNo = 0;

	//for(int i=0; i<nChallenges; i++) {
	//	//move up to the location:
	//	while(currentLocNo<nLStart[i]) {
	//		activeLocation = activeLocation->pActNext;
	//		currentLocNo++;
	//	}

	//	//and infect it:
	//	if(activeLocation->getS()>0) {
	//		activeLocation->haveEvent(0);
	//	}
	//}

	////TODO: Time between hurricanes:
	//timeNext += frequency;

	////because the hurricane has changed the landscape will need to
	////reset last event information to force update of whole landscape after intervention
	//world->bFlagNeedsFullRecalculation();

	return 1;
}

int InterHurricane::finalAction() {
	//printf("InterHurricane::finalAction()\n");
	
	return 1;
}

void InterHurricane::writeSummaryData(FILE *pFile) {
	
	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Ok\n\n");
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
