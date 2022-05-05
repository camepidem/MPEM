//InterThreatMap.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "Population.h"
#include "SummaryDataManager.h"
#include "ListManager.h"
#include "RasterHeader.h"
#include "myRand.h"
#include "InterThreatMap.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterThreatMap::InterThreatMap() {

	enabled = 0;

	timeFirst = world->timeEnd + 1e30;
	timeNext = timeFirst;

}

InterThreatMap::~InterThreatMap() {

}

int InterThreatMap::intervene() {
	return 1;
}

int InterThreatMap::infectCurrentLoc() {
	return 1;
}

int InterThreatMap::finalAction() {
	return 1;
}

int InterThreatMap::inBounds(LocationMultiHost *loc) {
	return 1;
}

void InterThreatMap::allocateStorage() {
	return;
}

void InterThreatMap::resetStorage(int iElement) {
	return;
}

void InterThreatMap::estimateTotalCost() {
	return;
}

void InterThreatMap::getStartingData() {
	return;
}

void InterThreatMap::selectNextLocation() {
	return;
}


void InterThreatMap::writeFileHeaders() {
	return;
}

void InterThreatMap::writeDataLine() {
	return;
}

void InterThreatMap::writeDataFixed() {
	return;
}

void InterThreatMap::writeArrayLine(char *szFileName, double *pArray) {
	return;
}

void InterThreatMap::writeArrayLine(char *szFileName, int *pArray) {
	return;
}

void InterThreatMap::writeSummaryData(FILE *pFile) {
	return;
}
