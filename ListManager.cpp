//ListManager.cpp

#include "stdafx.h"
#include "PopulationCharacteristics.h"
#include "Population.h"
#include "LocationMultiHost.h"
#include "Raster.h"
#include "ListManager.h"

LinkedList<LocationMultiHost> **ListManager::locationLists;
LinkedList<Population> ***ListManager::populationLists;

ListManager::ListManager()
{

	//Memroy housekeeping:
	locationLists = NULL;
	populationLists = NULL;

	//Constructs lists and forwards them to the Locations and Populations
	int nPop = PopulationCharacteristics::nPopulations;

	//Locations:
	locationLists = new LinkedList<LocationMultiHost>*[nLists];
	for(int j=0; j<nLists; j++) {
		locationLists[j] = new LinkedList<LocationMultiHost>();
	}

	LocationMultiHost::pLocationLists = locationLists;

	//Populations:
	populationLists = new LinkedList<Population>**[nPop];

	for(int i=0; i<nPop; i++) {
		populationLists[i] = new LinkedList<Population>*[nLists];
		for(int j=0; j<nLists; j++) {
			populationLists[i][j] = new LinkedList<Population>();
		}
		Population::pGlobalPopStats[i]->pPopulationLists = populationLists[i];
	}


}

ListManager::~ListManager()
{

	if(locationLists != NULL) {
		for(int j=0; j<nLists; j++) {
			delete locationLists[j];
		}
		delete [] locationLists;
		locationLists = NULL;
	}

	if(populationLists != NULL) {
		for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
			for(int j=0; j<nLists; j++) {
				delete populationLists[i][j];
			}
			delete [] populationLists[i];
		}
		delete [] populationLists;
		populationLists = NULL;
	}

}

double ListManager::weightedSumOfLocationListElements(int iList, Raster<double> *weightingRaster) {
	
	double dweightedSum = 0.0;
	
	ListNode<LocationMultiHost> *pLoc = locationLists[iList]->pFirst->pNext;
	while(pLoc != NULL) {

		dweightedSum += weightingRaster->getValue(pLoc->data->xPos,pLoc->data->yPos);

		pLoc = pLoc->pNext;
	}

	return dweightedSum;

}

double ListManager::weightedSumOfPopulationListElements(int iPop, int iList, Raster<double> *weightingRaster) {

	double dweightedSum = 0.0;
	
	ListNode<Population> *pLoc = populationLists[iPop][iList]->pFirst->pNext;
	while(pLoc != NULL) {

		dweightedSum += weightingRaster->getValue(pLoc->data->pParentLocation->xPos,pLoc->data->pParentLocation->yPos);

		pLoc = pLoc->pNext;
	}

	return dweightedSum;

}

double ListManager::zonedSumOfLocationListElements(int iList, const Raster<int> &zonesRaster, int zoneID) {
	double dZoneSum = 0.0;

	ListNode<LocationMultiHost> *pLoc = locationLists[iList]->pFirst->pNext;
	while (pLoc != NULL) {

		if (zonesRaster(pLoc->data->xPos, pLoc->data->yPos) == zoneID) {
			dZoneSum++;
		}

		pLoc = pLoc->pNext;
	}

	return dZoneSum;
}

double ListManager::zonedSumOfPopulationListElements(int iPop, int iList, const Raster<int> &zonesRaster, int zoneID) {
	double dZoneSum = 0.0;

	ListNode<Population> *pLoc = populationLists[iPop][iList]->pFirst->pNext;
	while (pLoc != NULL) {

		if (zonesRaster(pLoc->data->pParentLocation->xPos, pLoc->data->pParentLocation->yPos) == zoneID) {
			dZoneSum++;
		}

		pLoc = pLoc->pNext;
	}

	return dZoneSum;
}
