//ParameterDistributionUniform.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "Landscape.h"
#include "RateManager.h"
#include "DispersalManager.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "ParameterDistributionUniform.h"

#pragma warning(disable : 4996)

ParameterDistributionUniform::ParameterDistributionUniform() {

}

ParameterDistributionUniform::~ParameterDistributionUniform() {

}

int ParameterDistributionUniform::init(int iDist, Landscape *pWorld) {
	
	iDistribution = iDist;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szKey[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "ParameterDistribution_%d", iDistribution);

	printToKeyFile("//Only should control one of the below:\n");

	bControlsRateConstant = 0;

	//Read which rate(s) to modulate:
	char szRateConstant[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szKey, "%s_RateConstant", szRoot);
	RateManager::getRateConstantNameFromPopAndEvent(0,0,szRateConstant);
	if(readValueToProperty(szKey, szRateConstant, -2, "{#RateConstants#}")) {

		bControlsRateConstant = 1;
		
	}

	bControlsKernel = 0;
	iKernelToControl = 0;
	sprintf(szKey, "%s_ControlKernel", szRoot);
	if(readValueToProperty(szKey, &iKernelToControl, -2, "[0,#nKernels#)")) {
		
		bControlsKernel = 1;

	}

	//For now just modify single rate:
	dParameterMin = 0.1;
	sprintf(szKey, "%s_RateConstantMin", szRoot);
	readValueToProperty(szKey, &dParameterMin, 2, ">0");

	dParameterMax = 1.0;
	sprintf(szKey, "%s_RateConstantMax", szRoot);
	readValueToProperty(szKey, &dParameterMax, 2, ">0");


	bUseLogSampling = 0;
	sprintf(szKey, "%s_SampleLogSpace", szRoot);
	readValueToProperty(szKey, &bUseLogSampling, -2, "[0,1]");

	//Initialisation:
	if(!world->bReadingErrorFlag && !world->bGiveKeys) {

		if(bControlsKernel && bControlsRateConstant) {
			reportReadError("ERROR: Cannot control both KernelParameter and Rate constant simultaneously with single ParameterDistributionUniform\n");
			return 0;
		}

		if(bControlsRateConstant) {

			int iTargetPop = -1;
			int iTargetEvent = -1;
			if(!RateManager::getPopAndEventFromRateConstantName(szRateConstant, iTargetPop, iTargetEvent)) {
				reportReadError("ERROR: %s unable to find rate constant %s\n", szRoot, szRateConstant);
			}

			//Check valid to modify:
			if(iTargetPop < 0 || iTargetPop > PopulationCharacteristics::nPopulations) {
				reportReadError("ERROR: In %s Target population %d is invalid\n", szRoot, iTargetPop);
			} else {

				if(iTargetEvent < 0 || iTargetEvent > Population::pGlobalPopStats[iTargetPop]->nEvents) {
					reportReadError("ERROR: In %s Target Event %d is invalid\n", szRoot, iTargetEvent);
				}

			}

			if(!world->bReadingErrorFlag) {

				iRateConstantsControlledPop = iTargetPop;
				iRateConstantsControlledEvent = iTargetEvent;

			}

		}

		if(bControlsKernel) {
			if(iKernelToControl < 0 || iKernelToControl > DispersalManager::nKernels) {
				reportReadError("ERROR: Kernel %d does not exist: valid range: %d to %d\n", iKernelToControl, 0, DispersalManager::nKernels);
			}
		}

		if(bUseLogSampling) {
			dParameterMin = log(dParameterMin);
			dParameterMax = log(dParameterMax);
		}

	}


	return 1;
}

int ParameterDistributionUniform::sample() {

	double dSampledParameter = dParameterMin + dUniformRandom()*(dParameterMax - dParameterMin);

	if(bUseLogSampling) {
		dSampledParameter = exp(dSampledParameter);
	}

	logParameters(dSampledParameter);

	if(bControlsRateConstant) {
		world->rates->changeRateConstantDefault(iRateConstantsControlledPop, iRateConstantsControlledEvent, dSampledParameter);
	} else {
		//TODO: Fix...
		char szKey[_N_MAX_STATIC_BUFF_LEN];
		sprintf(szKey, "Kernel_%d_Parameter", iKernelToControl);
		DispersalManager::modifyKernelParameter(szKey, dSampledParameter);
	}

	return 1;
}

int ParameterDistributionUniform::logParameters(double dValue) {

	char szLogFileName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szLogFileName, "%sParameterDistribution_%d_Log%s",szPrefixOutput(),iDistribution,world->szSuffixTextFile);

	FILE *pLogFile = fopen(szLogFileName, "w");

	char szRateConstantName[_N_MAX_STATIC_BUFF_LEN];

	if(bControlsRateConstant) {
		world->rates->getRateConstantNameFromPopAndEvent(iRateConstantsControlledPop, iRateConstantsControlledEvent, szRateConstantName);
	} else {
		sprintf(szRateConstantName, "Kernel_%d_Parameter", iKernelToControl);
	}

	fprintf(pLogFile, "%s\n", szRateConstantName);
	fprintf(pLogFile, "%f\n", dValue);

	fclose(pLogFile);

	return 1;
}
