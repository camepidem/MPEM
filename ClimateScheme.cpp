//ClimateScheme.cpp

#include "stdafx.h"
#include "Landscape.h"
#include "ClimateScheme.h"

ClimateScheme::ClimateScheme() {

}

ClimateScheme::~ClimateScheme() {
	
}

int ClimateScheme::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	//Should not be calling this directly...

	return -1;
}

double ClimateScheme::getNextTime() {
	return 3155690000.0;
}


int ClimateScheme::applyNextTransition() {
	return -1;
}

int ClimateScheme::reset() {
	return 0;
}

static ClimateScheme *newScript(char scriptID) {
	//TODO: implement script factory properly...
	
	return NULL;
}
