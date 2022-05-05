//InterVariableBackgroundInfection.h

//Contains the data for the Climate Intervention

#ifndef INTERVARIABLEBACKGROUNDINFECTION_H
#define INTERVARIABLEBACKGROUNDINFECTION_H 1

#include "Intervention.h"

enum scripts {RATESCRIPT, KERNELSCRIPT, SCRIPTS_MAX};

class InterVariableBackgroundInfection : public Intervention {

public:

	InterVariableBackgroundInfection();

	~InterVariableBackgroundInfection();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

	int reset();

	int execute(int iArg);

protected:

	//Similar to climate

	/*

	Has a script of changing betas and kernels

	swaps at appropriate times, updates rate constant in ratemanager

	submits total rate of stuff happening to ratemanager as an external event -> pass function pointer? external events all intervention driven (probably good system...)?

	ratemanager calls a function on this *somehow* when selected as event 

	*/

	ClimateSchemeScriptDouble *rateScript;

	ClimateSchemeScriptKernel *kernelScript;

	int getGlobalNextTime();

	int iExternalEventIndex;

	int iCurrentScript;
	int nScripts;
	vector<ClimateScheme *> scripts;

	double dKernelZoomLevel;

};

#endif
