//CParameterPosterior.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "json/json.h"
#include "RateStructure.h"
#include "RateTree.h"
#include "CParameterPosterior.h"

#pragma warning(disable : 4996)

CParameterPosterior::CParameterPosterior() {

	//Memory housekeeping:
	rsdDensityArray				= NULL;

}

CParameterPosterior::~CParameterPosterior() {
	
	if(rsdDensityArray != NULL) {
		delete rsdDensityArray;
		rsdDensityArray = NULL;
	}

}

int CParameterPosterior::init(char *fromFile) {
	
	string line;
	string dump;
	stringstream ssline;

	if(!readIntFromCfg(fromFile, "nParameters", &nParameters)) {//TODO: Remove this manually specified key and read the file to determine it
		reportReadError("ERROR: Unable to read key nParameters from file %s\n", fromFile);
		return 0;
	}

	if(nParameters <= 0) {
		reportReadError("ERROR: nParameters in file %s <= 0.\n", fromFile);
		return 0;
	}

	if(!readIntFromCfg(fromFile, "nEntries", &nEntries)) {//TODO: Remove this manually specified key and read the file to determine it
		reportReadError("ERROR: Unable to read key nEntries from file %s\n", fromFile);
		return 0;
	}

	if(nEntries <= 0) {
		reportReadError("ERROR: nEntries in file %s <= 0.\n", fromFile);
		return 0;
	}

	char szKey[_N_MAX_STATIC_BUFF_LEN];

	////Box sizes:
	//pdParameterIntervalWidths = new double[nParameters];
	//for(int iParameter=0; iParameter<nParameters; iParameter++) {
	//	sprintf(szKey, "Parameter_%d_Width", iParameter);
	//	pdParameterIntervalWidths[iParameter] = 0.0;
	//	readDoubleFromCfg(fromFile, szKey, &pdParameterIntervalWidths[iParameter]);
	//}

	//LogSampling:
	pbSampleLogSpace.resize(nParameters);
	for(int iParameter=0; iParameter<nParameters; iParameter++) {
		sprintf(szKey, "Parameter_%d_SampleLogSpace", iParameter);
		pbSampleLogSpace[iParameter] = 0;
		readIntFromCfg(fromFile, szKey, &pbSampleLogSpace[iParameter]);
	}

	ifstream myFile;
	myFile.open(fromFile);
	
	if(myFile.is_open()) {
		
		//Skip header lines until find 'density' :
		int bFoundHeaderLine = 0;
		while(getline(myFile,line)) {

			char upperChars[_N_MAX_STATIC_BUFF_LEN];
			strcpy(upperChars, line.c_str());

			makeUpper(upperChars);

			string UCLine(upperChars);
			
			if(UCLine.find("DENSITY") != string::npos) {
				bFoundHeaderLine = 1;
				break;
			}
		}

		//Header line:
		pszParameterNames.resize(nParameters);
		if(bFoundHeaderLine) {
			ssline.clear();
			ssline << line;

			ssline >> dump;//Density header
			
			for(int iParameter=0; iParameter<nParameters; iParameter++) {
				if(ssline >> dump) {
					pszParameterNames[iParameter] = dump;
				} else {
					reportReadError("ERROR: Unable to read parameter name %d in [0, %d] from file %s\n", iParameter, nParameters, fromFile);
					return 0;
				}
			}

		} else {
			reportReadError("ERROR: Unable to read header line from file %s\n", fromFile);
			return 0;
		}


		//Main matrix:
		rsdDensityArray = new RateTree(nEntries);
		ppdParameterIntervalLowers.resize(nEntries);
		ppdParameterIntervalUppers.resize(nEntries);
		for(int iEntry=0; iEntry<nEntries; iEntry++) {
			if(getline(myFile,line)) {

				ssline.clear();
				ssline << line;

				double dTemp;

				//Density:
				if(ssline >> dTemp) {
					rsdDensityArray->submitRate(iEntry, dTemp);
				} else {
					reportReadError("ERROR: Unable to read density element from entry line %d from file %s\n", iEntry, fromFile);
					return 0;
				}

				//Parameter coordinates:
				ppdParameterIntervalLowers[iEntry].resize(nParameters);
				ppdParameterIntervalUppers[iEntry].resize(nParameters);
				for(int iParameter=0; iParameter<nParameters; iParameter++) {
					if(ssline >> dTemp) {

						if (dTemp < 0.0) {
							reportReadError("ERROR: Parameter element %d Lower value from entry line %d (%f) from file %s is less than zero\n", iParameter, iEntry, dTemp, fromFile);
							return 0;
						}

						ppdParameterIntervalLowers[iEntry][iParameter] = dTemp;
						if(ssline >> dTemp) {

							if (dTemp < 0.0) {
								reportReadError("ERROR: Parameter element %d Upper value from entry line %d (%f) from file %s is less than zero\n", iParameter, iEntry, dTemp, fromFile);
								return 0;
							}

							if(dTemp >= ppdParameterIntervalLowers[iEntry][iParameter]) {
								ppdParameterIntervalUppers[iEntry][iParameter] = dTemp;
							} else {
								reportReadError("ERROR: Parameter element %d Upper value from entry line %d from file %s is less than corresponding Lower value\n", iParameter, iEntry, fromFile);
								return 0;
							}
						} else {
							reportReadError("ERROR: Unable to read Parameter element %d Upper value from entry line %d from file %s\n", iParameter, iEntry, fromFile);
							return 0;
						}
					} else {
						reportReadError("ERROR: Unable to read Parameter element %d Lower value from entry line %d from file %s\n", iParameter, iEntry, fromFile);
						return 0;
					}
				}

			} else {
				reportReadError("ERROR: Unable to read entry line %d, expected [0, %d] from file %s\n", iEntry, nEntries, fromFile);
				return 0;
			}
		}

		myFile.close();

		//Parameter dependencies:
		char szParamDepJSON[_N_MAX_STATIC_BUFF_LEN];
		if (readStringFromCfg(fromFile, "ParameterDependencies", szParamDepJSON)) {
			Json::Reader reader;
			Json::Value root;

			if (!reader.parse(szParamDepJSON, root)) {
				reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s, message: %s\n", fromFile, szParamDepJSON, reader.getFormattedErrorMessages().c_str());
				return 0;
			}

			//Config structure:
			//Store as dependent parameters and what they depend on pairings
			//Ensures that we can't specify something depends on two things
			//Dependent_1 : {Main_1 : Multiplier_1}
			//...
			//Dependent_N : {Main_N : Multiplier_N}

			//Internal Structure:
			//Store as pairings of (things that have dependencies) and (all their respective dependencies)
			//Ensures that we can update all dependencies when a parameter changes
			//Main_1 : {Dependent_1 : Multiplier_1, Dependent_K : Multiplier_K, ...}
			//...
			//Main_N : {Dependent_N: Multiplier_N}

			if (!root.isObject()) {
				reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s. Must be a JSON object\n", fromFile, szParamDepJSON);
				return 0;
			}

			for (auto itr = root.begin(); itr != root.end(); ++itr) {
				//std::cout << itr.key() << " : " << *itr << std::endl;

				if (!itr.key().isString()) {
					reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s. Parameter names must be strings\n", fromFile, szParamDepJSON);
					return 0;
				}

				if (!(*itr).isObject()) {
					reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s. Parameter dependency for %s must be a JSON object\n", fromFile, szParamDepJSON, itr.key().asString().c_str());
					return 0;
				}

				auto parentParamJson = *itr;

				if (parentParamJson.size() != 1) {
					reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s. Parameter dependency for %s must have exactly one parent parameter, got: %d\n", fromFile, szParamDepJSON, itr.key().asString().c_str(), parentParamJson.size());
					return 0;
				}

				for (auto paramItr = parentParamJson.begin(); paramItr != parentParamJson.end(); ++paramItr) {

					if (!paramItr.key().isString()) {
						reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s. Parent parameter name for %s must be a string\n", fromFile, szParamDepJSON, itr.key().asString().c_str());
						return 0;
					}

					if (!(*paramItr).isDouble()) {
						reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s. Parent parameter multiplier for %s must be a double\n", fromFile, szParamDepJSON, itr.key().asString().c_str());
						return 0;
					}

					std::string mainParamName = paramItr.key().asString();

					std::string dependentParamName = itr.key().asString();

					double multiplier = (*paramItr).asDouble();

					if (multiplier < 0.0) {
						reportReadError("ERROR: Unable to parse ParameterDependencies in posterior file %s json string : %s. Parent parameter multiplier for %s must be non negative\n", fromFile, szParamDepJSON, mainParamName.c_str());
						return 0;
					}

					if (parameterCorrelations.find(mainParamName) == parameterCorrelations.end()) {
						std::map<std::string, double> paramData;

						paramData[dependentParamName] = multiplier;

						parameterCorrelations[mainParamName] = paramData;
					} else {
						//Not possible to specify in the JSON that a parameter is dependent on multiple main parameters, so don't need to check, and can just stick it in with no fear of overwriting
						parameterCorrelations[mainParamName][dependentParamName] = multiplier;
					}

				}

			}

			//Can't do much error checking here, as this class doesn't know what parameters are valid, this needs to be done in the class calling this
		}

		//Buffer for draws:
		pdParameterDraw.resize(nParameters);

	} else {
		reportReadError("ERROR: Unable to open file %s\n", fromFile);
		return 0;
	}


	return 1;
}

int CParameterPosterior::write(char *toFile) {
	
	//TODO:

	return 1;
}

double *CParameterPosterior::pdDrawFromPosterior() {

	double dTotalDensity = rsdDensityArray->getTotalRate();

	dTotalDensity *= dUniformRandom();

	int iEntry = rsdDensityArray->getLocationEventIndex(dTotalDensity);

	for(int iParameter=0; iParameter<nParameters; iParameter++) {

		double dLower = ppdParameterIntervalLowers[iEntry][iParameter];
		double dUpper = ppdParameterIntervalUppers[iEntry][iParameter];

		if(pbSampleLogSpace[iParameter]) {
			dLower = log(dLower);
			dUpper = log(dUpper);
		}

		double dDraw = dLower + (dUpper - dLower)*dUniformRandom();

		if(pbSampleLogSpace[iParameter]) {
			dDraw = exp(dDraw);
		}

		pdParameterDraw[iParameter] = dDraw;
	}

	return &pdParameterDraw[0];

}
