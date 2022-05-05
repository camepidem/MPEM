//ParameterDistributionPosterior.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "Landscape.h"
#include "RateManager.h"
#include "DispersalManager.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "CParameterPosterior.h"
#include "ParameterDistributionPosterior.h"

#pragma warning(disable : 4996)

ParameterDistributionPosterior::ParameterDistributionPosterior() {

	//Memory:
	pPosterior	= NULL;

}

ParameterDistributionPosterior::~ParameterDistributionPosterior() {
	
	if(pPosterior != NULL) {
		delete pPosterior;
		pPosterior = NULL;
	}

}

int ParameterDistributionPosterior::init(int iDist, Landscape *pWorld) {
	
	iDistribution = iDist;

	world = pWorld;

	char szRoot[_N_MAX_STATIC_BUFF_LEN];
	char szKey[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szRoot, "ParameterDistribution_%d_", iDistribution);
	sprintf(szKey, "%sPosteriorFileName", szRoot);
	sprintf(szPosteriorFileName, "P_PosteriorDistribution_%d.txt", iDistribution);
	readValueToProperty(szKey, szPosteriorFileName, -2, "#FileName#");

	//Initialisation:
	if(!world->bReadingErrorFlag && !world->bGiveKeys) {

		pPosterior = new CParameterPosterior();

		if(!pPosterior->init(szPosteriorFileName)) {
			reportReadError("ERROR: %s failed to initialise from %s\n", szRoot, szPosteriorFileName);
			return 0;
		}
		
		//Check all parameters are valid ones to modify:
		
		for (int iParameter = 0; iParameter < pPosterior->nParameters; iParameter++) {
			//Apply each parameter:
			//Check if for Rate constant or kernel:
			if (!validateParameterName(pPosterior->pszParameterNames[iParameter].c_str())) {
				reportReadError("ERROR: %s found invalid parameter names in %s\n", szRoot, szPosteriorFileName);
			}
		}

		auto &correlationData = pPosterior->parameterCorrelations;

		for (auto const &mainParamData : correlationData) {

			auto mainParamName = mainParamData.first;

			if (!validateParameterName(mainParamName.c_str())) {
				reportReadError("ERROR: %s found invalid parent parameter name \"%s\" in %s\n", szRoot, mainParamName.c_str(), szPosteriorFileName);
			}

			//Main parameters must be in the set of parameters that the posterior modifies:
			bool bFoundParam = false;
			for (int iParameter = 0; iParameter < pPosterior->nParameters; iParameter++) {
				if (compareParamNames(mainParamName.c_str(), pPosterior->pszParameterNames[iParameter].c_str())) {
					bFoundParam = true;
				}
			}

			if (!bFoundParam) {
				reportReadError("ERROR: %s specified parent parameter \"%s\" is not present in list of parameters controlled by the posterior distribution\n", szRoot, mainParamName.c_str(), szPosteriorFileName);
			}

			auto &mainParamDependencies = mainParamData.second;

			for (auto const &depParamData : mainParamDependencies) {

				auto depParamName = depParamData.first;

				if (!validateParameterName(depParamName.c_str())) {
					reportReadError("ERROR: %s found invalid dependent parameter name \"%s\" in %s\n", szRoot, depParamName.c_str(), szPosteriorFileName);
				}

				//We can't have a dependent parameter that is a main parameter:
				if (correlationData.find(depParamName) != correlationData.end()) {
					reportReadError("ERROR: %s specified parent parameter \"%s\" is also a dependent parameter\n", szRoot, mainParamName.c_str());
				}

				//Also can't have a dependent parameter that is a posterior parameter
				bool bFoundParam = false;
				for (int iParameter = 0; iParameter < pPosterior->nParameters; iParameter++) {
					if (compareParamNames(depParamName.c_str(), pPosterior->pszParameterNames[iParameter].c_str())) {
						bFoundParam = true;
					}
				}

				if (bFoundParam) {
					reportReadError("ERROR: %s specified dependent parameter \"%s\" is also a posterior parameter\n", szRoot, mainParamName.c_str());
				}

			}

		}

	}


	return 1;
}

int ParameterDistributionPosterior::validateParameterName(const char * szParamName) {

	char szParameterName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szParameterName, "%s", szParamName);
	makeUpper(szParameterName);
	if (strstr(szParameterName, "RATE") != NULL) {
		//is a rate constant:
		int iPop;
		int iEvent;
		if (!RateManager::getPopAndEventFromRateConstantName(szParameterName, iPop, iEvent)) {
			reportReadError("ERROR: ParameterDistributionPosterior unable to find rate constant %s\n", szParameterName);
			return 0;
		}
	} else {
		//is a kernel parameter:
		if (strstr(szParameterName, "KERNEL") != NULL) {
			//ok
			if (!DispersalManager::isValidKernelParameter(szParameterName)) {
				reportReadError("ERROR: ParameterDistributionPosterior unable to find simulation parameter %s in %s\n", szParameterName, szPosteriorFileName);
				reportReadError("Valid parameters for Kernel are Kernel_*_Parameter and Kernel_*_WithinCellProportion\n");
				return 0;
			}
		} else {
			reportReadError("ERROR: ParameterDistributionPosterior unable to find simulation parameter %s in %s\n", szParameterName, szPosteriorFileName);
			reportReadError("Valid parameters to modify contain *Rate* or *Kernel*\n");
			return 0;
		}
	}

	return 1;
}

int ParameterDistributionPosterior::sample() {

	double *dSampledParameters = pPosterior->pdDrawFromPosterior();

	std::map<std::string, double> params;

	for (int iParameter = 0; iParameter < pPosterior->nParameters; iParameter++) {
		//Apply each parameter:
		auto newParams = applyParameterChange(pPosterior->pszParameterNames[iParameter].c_str(), dSampledParameters[iParameter]);

		params.insert(newParams.begin(), newParams.end());
	}

	logParameters(params);

	return 1;
}

std::map<std::string, double> ParameterDistributionPosterior::applyParameterChange(const char * szParamName, double dNewValue) {

	std::map<std::string, double> newParamValues;

	int allOk = changeParameter(szParamName, dNewValue);

	newParamValues[szParamName] = dNewValue;

	if (pPosterior->parameterCorrelations.find(std::string(szParamName)) != pPosterior->parameterCorrelations.end()) {
		
		for (auto &derivedData : pPosterior->parameterCorrelations[std::string(szParamName)]) {

			auto derivedParamName = derivedData.first;

			auto multiplier = derivedData.second;

			double dValue = dNewValue * multiplier;

			allOk *= changeParameter(derivedParamName.c_str(), dValue);

			newParamValues[derivedParamName] = dValue;
		}
	}

	if (!allOk) {
		reportReadError("ERROR: Errors were encountered while changing parameters\n");
		world->abortSimulation();
	}

	return newParamValues;
}

int ParameterDistributionPosterior::changeParameter(const char * szParamName, double dNewValue) {

	char szParameterName[_N_MAX_STATIC_BUFF_LEN];

	//Check if for Rate constant or kernel:
	sprintf(szParameterName, "%s", szParamName);
	makeUpper(szParameterName);
	if (strstr(szParameterName, "RATE") != NULL) {
		//is a rate constant:
		int iPop;
		int iEvent;
		RateManager::getPopAndEventFromRateConstantName(szParameterName, iPop, iEvent);
		return world->rates->changeRateConstantDefault(iPop, iEvent, dNewValue);//TODO: Perhaps do this with similar interface to the kernel one?
	} else {
		//is a kernel parameter:
		return DispersalManager::modifyKernelParameter(szParameterName, dNewValue);
	}

}

bool ParameterDistributionPosterior::compareParamNames(const char * szParamName1, const char * szParamName2) {

	char szCanonName1[_N_MAX_STATIC_BUFF_LEN];
	char szCanonName2[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szCanonName1, "%s", szParamName1);
	sprintf(szCanonName2, "%s", szParamName2);

	if (strcmp(szCanonName1, szCanonName2) == 0) {
		return true;
	}

	return false;
}

int ParameterDistributionPosterior::logParameters(std::map<std::string, double> params) {

	char szLogFileName[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szLogFileName, "%sParameterDistribution_%d_Log%s",szPrefixOutput(),iDistribution,world->szSuffixTextFile);

	//Get the parameters in a consistent order:
	std::vector<std::string> sortNames;
	for (auto &paramData : params) {
		sortNames.push_back(paramData.first);
	}
	std::sort(sortNames.begin(), sortNames.end());

	std::vector<double> values(sortNames.size());
	for (int iName = 0; iName < sortNames.size(); ++iName) {
		values[iName] = params[sortNames[iName]];
	}


	FILE *pLogFile = fopen(szLogFileName, "w");

	//Header:
	for(int iParameter = 0; iParameter < sortNames.size(); ++iParameter) {
		fprintf(pLogFile, "%s ", sortNames[iParameter].c_str());
	}
	fprintf(pLogFile, "\n");

	//Data:
	for(int iParameter = 0; iParameter < values.size(); ++iParameter) {
		fprintf(pLogFile, "%.12f ", values[iParameter]);
	}
	fprintf(pLogFile, "\n");

	fclose(pLogFile);

	return 1;
}

