//DispersalManager.h

#ifndef DISPERSALMANAGER_H
#define DISPERSALMANAGER_H 1

#include "classes.h"

class DispersalManager {

public:

	//Constructor/destructor:
	DispersalManager();
	~DispersalManager();

	static int setLandscape(Landscape *myWorld);

	static void init();

	static void reset();

	static int nKernels;

	static DispersalKernel *getKernel(int iForPop);

	static int getLongestKernelShortRange();

	static double dGetVSPatchScaling(int iPop);
	static int kernelLongTrial(int iFromPopType, double dXSource, double dYSource);

	static void writeSummaryData(char *szSummaryFile);

	static int64_t getNKernelLongAttempts(int iKernel);
	static int64_t getNKernelLongSuccess(int iKernel);
	static int64_t getNKernelLongExport(int iKernel);

	static int trialInfection(int iFromPopType, double dXPos, double dYPos);//attempt to throw virtual inoculum to a given location and cause infection

	//static int modifyKernelParameter(int iKernel, double dNewParameter);
	static int isValidKernelParameter(char *szKey);
	static int modifyKernelParameter(char *szKey, double dNewParameter);

protected:

	static int parseKernelParameter(char *szKey, int &iKernel, char *szParameterName=NULL);

	static Landscape *world;

	static vector<int> pPopToKernelIndexMap;

	static vector<DispersalKernel *> pRealKernels;

	static DispersalKernel *makeKernel(int iKernel);

	static double dVSHostscaling;//In order to have VS be consistent for small patches, need to VS from patches quantised at same resolution as smallest patch in landscape, but then have infections succeed at rate dVSHostscaling/(actual target patch size)

	static int bErrorFlag;

	static vector<int64_t> pnKLongAttempts;	//Total number of virtual sporulations
	static vector<int64_t> pnKLongSuccess;	//Total number of virtual spores that successfully cause infection
	static vector<int64_t> pnKLongExports;	//Total number of virtual spores that left the landscape entirely

	static char* blendedKernelKey;

	static vector<int> getKernelsForPop(int iPopulation); //Returns indices of kernels to be used by specified population - can be more than one due to blended kernels

	friend class DispersalKernelBlended;

};

#endif
