//Intervention.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "ProfileTimer.h"
#include "Intervention.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

Landscape* Intervention::world;

Intervention::Intervention() {
	//do nothing
}

Intervention::~Intervention() {
	//remove all dynamic data associated with this object
}

int Intervention::performIntervention() {

	ProfileTimer timer(timeSpent);

	int retVal = intervene();

	return retVal;
}

int Intervention::performReset() {

	ProfileTimer timer(timeSpent);

	int retVal = reset();

	return retVal;
}

int Intervention::performFinalAction() {

	ProfileTimer timer(timeSpent);

	int retVal = finalAction();

	return retVal;
}


int Intervention::intervene() {
	return 1;
}

int Intervention::reset() {
	
	if(enabled) {
		timeNext = timeFirst;
	}

	return enabled;
}

int Intervention::finalAction() {
	return 1;
}


int Intervention::disable() {
	
	enabled = 0;

	return enabled;
}

double Intervention::getTimeNext() {
	return timeNext;
}

int Intervention::execute(int iArg) {
	return 1;
}

int Intervention::setWorld(Landscape *myWorld) {
	world = myWorld;

	return 1;
}

void Intervention::writeSummaryData(FILE *pFile) {
	printf("Error: Should not use base class writeSummaryData function directly\n");
	return;
}
