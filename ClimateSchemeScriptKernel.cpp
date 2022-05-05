//ClimateSchemeScriptKernel.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "KernelRasterPoints.h"
#include "ClimateSchemeScriptKernel.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

ClimateSchemeScriptKernel::ClimateSchemeScriptKernel() {

}

ClimateSchemeScriptKernel::~ClimateSchemeScriptKernel() {

	map<string, int>::iterator iter;
	for (iter = mapKernelIdToIndex.begin(); iter != mapKernelIdToIndex.end(); iter++) {
		delete pKernels[iter->second];
	}

}

int ClimateSchemeScriptKernel::init(int i_Scheme, Landscape *pWorld) {

	iScheme = i_Scheme;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szFileName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "VariableBackgroundInfection_Kernels_%d", i_Scheme);
	sprintf(szFileName, "%s.txt", szRoot);

	//Initialisation:
	if(!world->bReadingErrorFlag && !world->bGiveKeys) {

		if(!readTimeStateIDPairsToUniqueMap(szFileName, nStates, pdTimes, vKernelIds, mapKernelIdToIndex)) {
			reportReadError("ERROR: %s unable to init from file %s\n", szRoot, szFileName);
		} else {
			//Read all the unique kernels:
			size_t nUniqueKernels = mapKernelIdToIndex.size();
			pKernels.resize(nUniqueKernels);
			map<string,int>::iterator iter;
			char sKernelName[_N_MAX_STATIC_BUFF_LEN];
			for(iter = mapKernelIdToIndex.begin(); iter != mapKernelIdToIndex.end(); iter++) {
				sprintf(sKernelName, "%s", iter->first.c_str());
				pKernels[iter->second] = new KernelRasterPoints(sKernelName);//Why doesn't iter->first.c_str() just work?
			}
		}

		if(!world->bReadingErrorFlag) {
			reset();
		}


	}

	return 1;
}

double ClimateSchemeScriptKernel::getNextTime() {
	return timeNext;
}


int ClimateSchemeScriptKernel::applyNextTransition() {

	//Don't actually do anything in here:
	if (currentState + 1 < pdTimes.size()) {
		currentState++;
		timeNext = pdTimes[currentState];
	} else {
		//Ran out of states, so remain in last state until end of simulation:
		timeNext = world->timeEnd + 1.0e30;
	}

	return 1;
}

KernelRaster *ClimateSchemeScriptKernel::getCurrentKernel() {
	if(currentState >= 0) {
		return pKernels[mapKernelIdToIndex[vKernelIds[currentState]]];
	} else {
		return NULL;
	}
}

int ClimateSchemeScriptKernel::reset() {

	//If there are no times:
	currentState = -1;
	timeFirst = world->timeEnd + 1.0e30;
	timeNext = timeFirst;

	if (pdTimes.size() > 0) {
		//Some valid times:
		currentState = 0;

		while (currentState < pdTimes.size() && pdTimes[currentState] < world->timeStart) {
			currentState++;
		}

		if (currentState < pdTimes.size()) {
			timeFirst = pdTimes[currentState];
			timeNext = timeFirst;
		} else {
			//There are no times after the start of the simulation...
			currentState = -1;
			printk("Warning: VariableBackgroundInfection_Kernels_%d specified %d times, but none of these times are after the simulation start time (%f). Last specified time %f.\n", iScheme, int(pdTimes.size()), world->timeStart, pdTimes[pdTimes.size() - 1]);
		}

	}

	return 1;
}
