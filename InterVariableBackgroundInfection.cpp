//InterVariableBackgroundInfection.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "RateManager.h"
#include "DispersalManager.h"
#include "myRand.h"
#include "RasterHeader.h"
#include "KernelRaster.h"
#include "ClimateSchemeScriptDouble.h"
#include "ClimateSchemeScriptKernel.h"
#include "InterVariableBackgroundInfection.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterVariableBackgroundInfection::InterVariableBackgroundInfection() {
	timeSpent = 0;
	InterName = "VariableBackgroundInfection";

	//Memory housekeeping:
	rateScript					= NULL;
	kernelScript				= NULL;


	setCurrentWorkingSubection(InterName, CM_APPLICATION);

	//Read Data:

	char szKey[_N_MAX_STATIC_BUFF_LEN];

	enabled = 0;
	sprintf(szKey, "%sEnable", InterName);
	readValueToProperty(szKey,&enabled,-1, "[0,1]");

	//TODO: ERROR HANDLING!
	if(enabled || world->bGiveKeys) {

		dKernelZoomLevel = 1.0;
		sprintf(szKey, "%sKernelZoomLevel", InterName);
		readValueToProperty(szKey,&dKernelZoomLevel,-2, ">0");

		if(world->bGiveKeys) {

			//TODO: Some useful information here:

		} else {

			//Allocate scripts

			nScripts = SCRIPTS_MAX;
			scripts.resize(nScripts);

			rateScript = new ClimateSchemeScriptDouble();
			rateScript->init(0, world);

			scripts[RATESCRIPT] = rateScript;


			kernelScript = new ClimateSchemeScriptKernel();
			kernelScript->init(0, world);

			scripts[KERNELSCRIPT] = kernelScript;

			//Register with the external event manager:
			iExternalEventIndex = world->rates->requestExternalRate(this);


		}

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}


	//Set up intervention:
	if(enabled && !world->bGiveKeys) {

		getGlobalNextTime();

	}

}

InterVariableBackgroundInfection::~InterVariableBackgroundInfection() {

	if(rateScript != NULL) {
		delete rateScript;
		rateScript = NULL;
	}

	if(kernelScript != NULL) {
		delete kernelScript;
		kernelScript = NULL;
	}

}

//This one for changing the rate constant and/or raster
int InterVariableBackgroundInfection::intervene() {
	//printf("WEATHER!\n");

	//Ensure have the right next event:
	getGlobalNextTime();


	//TODO: Apply the change in rate or kernel used
	if(iCurrentScript >=0) {
		scripts[iCurrentScript]->applyNextTransition();

		switch(iCurrentScript) {
		case RATESCRIPT:
			//Submit a new rate:
			world->rates->submitRateExternal(iExternalEventIndex, rateScript->getCurrentValue() / DispersalManager::dGetVSPatchScaling(0));//TODO: Modulate by nMaxHosts for consistency?
			break;
		case KERNELSCRIPT:
			//Don't need to do anything further, get kernel pointers as needed
			break;
		default:
			//Disaster...
			break;
		}

	}

	//Update time of next event:
	getGlobalNextTime();

	return 1;
}

//Function for having a sporulation event:
int InterVariableBackgroundInfection::execute(int iArg) {

	//Have a virtual sporulation:
	KernelRaster *pKernel = kernelScript->getCurrentKernel();

	if(pKernel != NULL) {

		//Sporulate from centre of cell:
		//Apparently all use cases have the KernelPoints file with cells registered at their centroids
		double xSource = 0.0;
		double ySource = 0.0;
		double dx, dy;
		pKernel->kernelVirtualSporulation(world->time, xSource, ySource, dx, dy);

		//Spread uniformly over a box of size dKernelZoomLevel rather than all the the centre of a point
		//The KernelPoints object already does this:
		//double xE = dUniformRandom() - 0.5;
		//double yE = dUniformRandom() - 0.5;

		DispersalManager::trialInfection(0, (xSource + dx)*dKernelZoomLevel, world->header->nRows - (ySource + dy)*dKernelZoomLevel);//Flip y coordinate to be consistent with other kernels - misery

	} else {
		printk("WARNING: %s requesting sporulation event at a time %.3f before a kernel is defined %.3f. Discarding event.\n", InterName, world->time, kernelScript->getNextTime());
	}

	return 1;
}

int InterVariableBackgroundInfection::reset() {

	if(enabled) {

		for(int iScript=0; iScript<nScripts; iScript++) {
			scripts[iScript]->reset();
		}

		getGlobalNextTime();

		//Set rate back to zero:
		world->rates->submitRateExternal(iExternalEventIndex, 0.0);

	}

	return enabled;
}

int InterVariableBackgroundInfection::getGlobalNextTime() {

	double dMinTime = world->timeEnd + 1e30;

	for(int iScript=0; iScript<nScripts; iScript++) {

		double dNextTime = scripts[iScript]->getNextTime();
		if(dNextTime < dMinTime) {
			dMinTime = dNextTime;
			iCurrentScript = iScript;
		}

	}

	timeNext = dMinTime;

	return 1;

}

int InterVariableBackgroundInfection::finalAction() {
	//printf("InterVariableBackgroundInfection::finalAction()\n");

	return 1;
}

void InterVariableBackgroundInfection::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Ok\n");
		fprintf(pFile, "Managed %d scripts\n\n", nScripts);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
