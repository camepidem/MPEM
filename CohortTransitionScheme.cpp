//CohortTransitionScheme.cpp

#include "stdafx.h"
#include "Landscape.h"
#include "CohortTransitionScheme.h"

CohortTransitionScheme::CohortTransitionScheme() {

}

CohortTransitionScheme::~CohortTransitionScheme() {

}

int CohortTransitionScheme::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	//Should not be calling this directly...

	return -1;
}

double CohortTransitionScheme::getNextTime() {
	return 3155690000.0;
}


int CohortTransitionScheme::applyNextTransition() {
	return -1;
}

int CohortTransitionScheme::reset() {
	return 0;
}
