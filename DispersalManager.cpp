//DispersalManager.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "SimulationProfiler.h"
#include "RasterHeader.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "Population.h"
#include "DispersalKernelRasterBased.h"
#include "DispersalKernelBlended.h"
#include "DispersalKernelFunctionalUSSOD.h"
#include "DispersalKernelQuarantineRobin.h"
#include "DispersalKernelTimeSpaceVarying.h"
#include "DispersalManager.h"
#include <inttypes.h>

#ifdef _WIN32
#define DLL_KERNELS_ENABLED
#endif

#ifdef DLL_KERNELS_ENABLED
#include <windows.h>
#include "Factory.h"
#endif

#pragma warning(disable : 4996)

Landscape* DispersalManager::world;
int DispersalManager::nKernels;
vector<int> DispersalManager::pPopToKernelIndexMap;
vector<DispersalKernel *> DispersalManager::pRealKernels;
double DispersalManager::dVSHostscaling;
int DispersalManager::bErrorFlag;
vector<int64_t> DispersalManager::pnKLongAttempts;
vector<int64_t> DispersalManager::pnKLongSuccess;
vector<int64_t> DispersalManager::pnKLongExports;
char* DispersalManager::blendedKernelKey = "BLENDED";

#ifdef DLL_KERNELS_ENABLED
typedef Factory * (__stdcall *DLLGETFACTORY)(void);
#endif

DispersalManager::DispersalManager() {

}

DispersalManager::~DispersalManager() {

	for (int i = 0; i < nKernels; i++) {
		delete pRealKernels[i];
	}

}

int DispersalManager::setLandscape(Landscape *myWorld) {
	world = myWorld;

	return 1;
}

void DispersalManager::init() {

	nKernels = 0;
	bErrorFlag = 0;


	int nMaxIndex = 0;
	if (!world->bGiveKeys) {
		pPopToKernelIndexMap.resize(PopulationCharacteristics::nPopulations);

		for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations; iPop++) {
			vector<int> popKernels = getKernelsForPop(iPop);

			pPopToKernelIndexMap[iPop] = popKernels[0]; // First one is always the actual kernel, either a normal kernel or the blended kernel

			for (auto iKernel : popKernels) {
				if (iKernel > nMaxIndex) {
					nMaxIndex = iKernel;
				}
			}
		}

		//Could have a second level of mapping, but I'll just
		//Check that do not have any unindexed kernels:
		for (int iKernel = 0; iKernel <= nMaxIndex; iKernel++) {

			int found = 0;

			for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations; iPop++) {

				vector<int> popKernels = getKernelsForPop(iPop);

				for (auto popKernel : popKernels) {
					if (popKernel == iKernel) {
						found = 1;
						break;
					}
				}
				if (found) {
					break;
				}
			}

			if (found == 0) {
				//Didnt find it:
				bErrorFlag = 1;
				world->bInitialisationOnTrack = 0;
				reportReadError("ERROR: Could not find Kernel %d\n", iKernel);
				return;
			}

		}
	}

	nKernels = nMaxIndex + 1;

	pRealKernels.resize(nKernels);

	//Attempt to create all kernels:
	for (int i = 0; i < nKernels; i++) {
		pRealKernels[i] = makeKernel(i);
	}

	if (bErrorFlag) {
		world->bInitialisationOnTrack = 0;
		reportReadError("ERROR: Unable to create Kernel\n");
		return;
	}

	//If creation successful, initialise Kernels:
	for (int i = 0; i < nKernels; i++) {
		if (!pRealKernels[i]->init(i)) {
			reportReadError("ERROR: Kernel %d did not successfully initialise.\n", i);
			bErrorFlag = 1;
		}
	}

	if (!world->bGiveKeys) {

		//Find minimum patch size:
		dVSHostscaling = world->getMinimumHostScaling();

		if (dVSHostscaling < 1e-3) {
			reportReadError("CRITICALLY LOW HOST SCALING: HOST LANDSCAPE IS NOT RESOLVED ENOUGH TO REPRESENT HOST DISTRIBUTION. AMEND VERY LOW HOST DENSITY CELLS\n");
			return;
		}

		//Initialise tracking of sporulation data:
		pnKLongAttempts.resize(nKernels);
		pnKLongSuccess.resize(nKernels);
		pnKLongExports.resize(nKernels);

		for (int i = 0; i < nKernels; i++) {
			pnKLongAttempts[i] = 0;
			pnKLongSuccess[i] = 0;
			pnKLongExports[i] = 0;
		}

	}

	if (bErrorFlag) {
		world->bInitialisationOnTrack = 0;
		reportReadError("ERROR: Kernel Initialisation failed\n");
		return;
	}

}

void DispersalManager::reset() {
	for (int i = 0; i < nKernels; i++) {
		pRealKernels[i]->reset();

		pnKLongAttempts[i] = 0;
		pnKLongSuccess[i] = 0;
		pnKLongExports[i] = 0;
	}
	return;
}

DispersalKernel * DispersalManager::getKernel(int iForPop) {
	return pRealKernels[pPopToKernelIndexMap[iForPop]];
}

int DispersalManager::getLongestKernelShortRange() {
	int nLongestKernelShortRange = 0;

	for (int iKernel = 0; iKernel < nKernels; iKernel++) {
		int nKernelShortRange = pRealKernels[iKernel]->getShortRange();
		if (nKernelShortRange > nLongestKernelShortRange) {
			nLongestKernelShortRange = nKernelShortRange;
		}
	}

	return nLongestKernelShortRange;
}

DispersalKernel *DispersalManager::makeKernel(int iKernel) {

	char szDLLFileName[_N_MAX_STATIC_BUFF_LEN];
	char szKeyName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szKeyName, "Kernel_%d_Source", iKernel);

	if (readStringFromCfg(world->szMasterFileName, szKeyName, szDLLFileName)) {
		//DLL source specified, attempt to create from DLL:

		//Special cases:
		if (strcmp("USSODCAUCHY", szDLLFileName) == 0) {
			return new DispersalKernelFunctionalUSSOD();
		}

		if (strcmp("ROBINQUARANTINE", szDLLFileName) == 0) {
			return new DispersalKernelQuarantineRobin();
		}

		if (strcmp("TIMEANDSPACEVARYING", szDLLFileName) == 0) {
			return new DispersalKernelTimeSpaceVarying();
		}

		if (strcmp(blendedKernelKey, szDLLFileName) == 0) {
			return new DispersalKernelBlended();
		}

#ifdef DLL_KERNELS_ENABLED

		//Do not allow two kernels to run from the same dll due to shared resources:
		//TODO: is this actually necessary?
		char szTestSourceName[_N_MAX_STATIC_BUFF_LEN];
		char szTestKeyName[_N_MAX_STATIC_BUFF_LEN];

		//Check if any other kernels have specified the same source:
		for (int i = 0; i < iKernel; i++) {
			sprintf(szKeyName, "Kernel_%d_Source", i);
			if (readStringFromCfg(world->szMasterFileName, szTestKeyName, szTestSourceName)) {
				if (strcmp(szTestSourceName, szDLLFileName) == 0) {
					reportReadError("ERROR: Unable to use the same DLL as a source for more than one kernel.\n Copy the DLL and rename it in order to use the DLL more than once\n");
					bErrorFlag = 1;
					return NULL;
				}
			}
		}


		//Need to convert to wide char version for windows load library:
		wchar_t *wcszDLLFileName = new wchar_t[256];
		mbstowcs(wcszDLLFileName, szDLLFileName, 256);

		HINSTANCE hInstDll = LoadLibrary(wcszDLLFileName);
		if (!hInstDll) {
			reportReadError("ERROR: Could not find DLL Kernel specified in file: %s\n", szDLLFileName);
			bErrorFlag = 1;
			return NULL;
		}

		DLLGETFACTORY pDllGetFactory = (DLLGETFACTORY)GetProcAddress(hInstDll, "returnFactory");
		// Create the object using the factory function
		Factory *pMyFactory = (pDllGetFactory)();
		if (pMyFactory == NULL) {
			reportReadError("ERROR: Unable to instantiate kernel from DLL %s\n", szDLLFileName);
			bErrorFlag = 1;
			return NULL;
		}
		DispersalKernel *d = (DispersalKernel *)pMyFactory->CreateProduct();

		setCurrentWorkingSection("DISPERSAL");

		return d;
#else
		reportReadError("ERROR: Specified a DLL Kernel with \"%s %s\".\nDLL Kernels are not supported on this platform\n", szKeyName, szDLLFileName);
		bErrorFlag = 1;
		return NULL;
#endif
	} else {
		//Create kernel from built in implementation:
		return new DispersalKernelRasterBased();
	}

}

vector<int> DispersalManager::getKernelsForPop(int iPopulation) {

	vector<int> kernels;

	int popKernel = Population::pGlobalPopStats[iPopulation]->iKernel;

	kernels.push_back(popKernel);

	//Special case for blended kernels:
	char szKeyName[_N_MAX_STATIC_BUFF_LEN];
	char szDLLFileName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szKeyName, "Kernel_%d_Source", popKernel);

	if (readStringFromCfg(world->szMasterFileName, szKeyName, szDLLFileName)) {
		if (strcmp(blendedKernelKey, szDLLFileName) == 0) {
			//TODO: Find out what kernels it uses
			vector<int> blendedKernelIndices;
			vector<double> blendedKernelWeights;

			int success = DispersalKernelBlended::getTargetKernelData(popKernel, blendedKernelIndices, blendedKernelWeights);
			if (success) {
				for (auto iKernel : blendedKernelIndices) {
					kernels.push_back(iKernel);
				}
			} else {
				reportReadError("ERROR: Blended kernel %d has incorrect configuration data\n", popKernel);
			}
		}
	}

	return kernels;
}

double DispersalManager::dGetVSPatchScaling(int iPop) {
	return dVSHostscaling;
}

int DispersalManager::kernelLongTrial(int iFromPopType, double dXSource, double dYSource) {

	world->profiler->kernelVSTimer.start();

	pnKLongAttempts[pPopToKernelIndexMap[iFromPopType]]++;

	double dDx, dDy;

	getKernel(iFromPopType)->KernelVirtualSporulation(world->time, dXSource, dYSource, dDx, dDy);

	trialInfection(iFromPopType, dXSource + dDx, dYSource + dDy);

	world->profiler->kernelVSTimer.stop();

	return 1;
}

int DispersalManager::trialInfection(int iFromPopType, double dXPos, double dYPos) {

	if (floor(dXPos) < 0.0 || floor(dXPos) >= world->header->nCols || floor(dYPos) < 0.0 || floor(dYPos) >= world->header->nRows) {
		//Spore fell outside the landscape:
		pnKLongExports[pPopToKernelIndexMap[iFromPopType]]++;
		return 0;
	}

	LocationMultiHost *testLoc = world->locationArray[int(floor(dXPos) + floor(dYPos)*world->header->nCols)];
	if (testLoc != NULL) {
		//Check if sporulation succeeded in causing an infection:
		pnKLongSuccess[pPopToKernelIndexMap[iFromPopType]] += testLoc->trialInfection(iFromPopType, dXPos - floor(dXPos), dYPos - floor(dYPos));
	}

	return 1;
}

int DispersalManager::parseKernelParameter(char *szKey, int &nKernel, char *szParameterName) {

	//Of form: Kernel_*iKernel*_*szParameterName
	char szTest[_N_MAX_STATIC_BUFF_LEN];
	char szCompare[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szCompare, "%s", szKey);
	makeUpper(szCompare);

	for (int iKernel = 0; iKernel < nKernels; iKernel++) {
		sprintf(szTest, "KERNEL_%d_", iKernel);
		if (strstr(szCompare, szTest) != NULL) {
			nKernel = iKernel;
			if (szParameterName != NULL) {
				sprintf(szParameterName, "%s", strstr(szCompare, szTest) + strlen(szTest));
			}
			return 1;
		}
	}

	return 0;

}

int DispersalManager::isValidKernelParameter(char *szKey) {

	int nKernel;
	char szParameterName[_N_MAX_STATIC_BUFF_LEN];

	parseKernelParameter(szKey, nKernel, szParameterName);

	return pRealKernels[nKernel]->changeParameter(szParameterName, 1.0, 1);

}

int DispersalManager::modifyKernelParameter(char *szKey, double dNewParameter) {

	int nKernel;
	char szParameterName[_N_MAX_STATIC_BUFF_LEN];

	if (parseKernelParameter(szKey, nKernel, szParameterName)) {

		if (pRealKernels[nKernel]->changeParameter(szParameterName, dNewParameter)) {
			world->bFlagNeedsInfectionRateRecalculation();
			return 1;
		}

	}

	return 0;

}

//int DispersalManager::modifyKernelParameter(int iKernel, double dNewParameter) {
//	
//	if(pRealKernels[iKernel]->changeParameter(dNewParameter)) {
//		world->bFlagNeedsFullRecalculation();
//	}
//	
//	return 1;
//}

void DispersalManager::writeSummaryData(char *szSummaryFile) {

	printHeaderText(szSummaryFile, "Dispersal Kernel:");

	for (int i = 0; i < nKernels; i++) {
		FILE *pSummaryFile = fopen(szSummaryFile, "a");
		fprintf(pSummaryFile, "\nKernel_%d:\n\n", i);
		fclose(pSummaryFile);
		pRealKernels[i]->writeSummaryData(szSummaryFile);
	}

	FILE *pSummaryFile = fopen(szSummaryFile, "a");
	fprintf(pSummaryFile, "\nVirtual Sporulation Summary:\n");
	fprintf(pSummaryFile, "VSPatchScaling         %15g\n", double(dGetVSPatchScaling(0)));
	fprintf(pSummaryFile, "Total:\n");
	fprintf(pSummaryFile, "Attempts          %20" PRId64 "\n", getNKernelLongAttempts(-1));
	fprintf(pSummaryFile, "Successes         %20" PRId64 "\n", getNKernelLongSuccess(-1));
	fprintf(pSummaryFile, "Exports           %20" PRId64 "\n", getNKernelLongExport(-1));
	if (getNKernelLongAttempts(-1) > 0) {
		fprintf(pSummaryFile, "Success Rate           %18.2f%c\n", 100.0 * double(getNKernelLongSuccess(-1)) / double(getNKernelLongAttempts(-1)), '%');
		fprintf(pSummaryFile, "Export Rate            %18.2f%c\n", 100.0 * double(getNKernelLongExport(-1)) / double(getNKernelLongAttempts(-1)), '%');
	} else {
		fprintf(pSummaryFile, "Success Rate           %15s\n", "NONE");
		fprintf(pSummaryFile, "Export Rate            %15s\n", "NONE");
	}
	fprintf(pSummaryFile, "\nIndividual Kernel Breakdown:\n");
	for (int i = 0; i < nKernels; i++) {
		fprintf(pSummaryFile, "Kernel_%d:\n", i);
		//Give results relative to nMaxHosts of source population so results are comparable for the same parameter sets:
		fprintf(pSummaryFile, "Kernel_%d_Attempts           %11.0f\n", i, double(getNKernelLongAttempts(i)) / double(PopulationCharacteristics::nMaxHosts));
		fprintf(pSummaryFile, "Kernel_%d_Successes          %11.0f\n", i, double(getNKernelLongSuccess(i)) / double(PopulationCharacteristics::nMaxHosts));
		fprintf(pSummaryFile, "Kernel_%d_Exports            %11.0f\n", i, double(getNKernelLongExport(i)) / double(PopulationCharacteristics::nMaxHosts));
		fprintf(pSummaryFile, "\n");
	}


	fclose(pSummaryFile);
}

int64_t DispersalManager::getNKernelLongAttempts(int iKernel) {
	int64_t nKLongAttempts = 0;

	if (iKernel >= 0) {
		nKLongAttempts = pnKLongAttempts[iKernel];
	} else {
		//Add up all sporulations:
		for (int i = 0; i < nKernels; i++) {
			nKLongAttempts += pnKLongAttempts[i];
		}
	}

	return nKLongAttempts;
}

int64_t DispersalManager::getNKernelLongSuccess(int iKernel) {
	int64_t nKLongSuccess = 0;

	if (iKernel >= 0) {
		nKLongSuccess = pnKLongSuccess[iKernel];
	} else {
		//Add up all successful sporulations:
		for (int i = 0; i < nKernels; i++) {
			nKLongSuccess += pnKLongSuccess[i];
		}
	}

	return nKLongSuccess;
}

int64_t DispersalManager::getNKernelLongExport(int iKernel) {
	int64_t nKLongExport = 0;

	if (iKernel >= 0) {
		nKLongExport = pnKLongExports[iKernel];
	} else {
		//Add up all successful sporulations:
		for (int i = 0; i < nKernels; i++) {
			nKLongExport += pnKLongExports[i];
		}
	}

	return nKLongExport;
}
