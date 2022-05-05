//ClimateSchemeScriptKernel.h

#pragma once

#include "ClimateScheme.h"

class ClimateSchemeScriptKernel : public ClimateScheme {

public:

	ClimateSchemeScriptKernel();
	~ClimateSchemeScriptKernel();

	int init(int i_Scheme, Landscape *pWorld);

	int applyNextTransition();

	double getNextTime();

	int reset();

	KernelRaster *getCurrentKernel();

protected:

	int currentState;

	int nStates;
	vector<double> pdTimes;
	vector<string> vKernelIds;

	map<string, int> mapKernelIdToIndex;
	vector<KernelRaster *> pKernels;

	double timeFirst;
	double timeNext;

};
