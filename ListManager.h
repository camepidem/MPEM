//ListManager.h

#ifndef LISTMANAGER_H
#define LISTMANAGER_H 1

#include "classes.h"
#include "LinkedList.h"

class ListManager
{


public:

	ListManager();
	~ListManager();

	//Lists for the Location statistics:
	static LinkedList<LocationMultiHost> **locationLists;

	//Array of Lists for each separate population:
	static LinkedList<Population> ***populationLists;

	static const int iActiveList		= 0;
	static const int iSusceptibleList	= 1;
	static const int iInfectiousList	= 2;
	static const int iTouchedList		= 3;

	static const int nLists				= 4;

	static double weightedSumOfLocationListElements(int iList, Raster<double> *weightingRaster);
	static double weightedSumOfPopulationListElements(int iPop, int iList, Raster<double> *weightingRaster);

	static double zonedSumOfLocationListElements(int iList, const Raster<int> &zonesRaster, int zoneID);
	static double zonedSumOfPopulationListElements(int iPop, int iList, const Raster<int> &zonesRaster, int zoneID);

protected:


};

#endif
