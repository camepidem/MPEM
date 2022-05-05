//InterParameterDistribution.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "myRand.h"
#include "ParameterDistribution.h"
#include "ParameterDistributionUniform.h"
#include "ParameterDistributionPosterior.h"
#include "InterParameterDistribution.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterParameterDistribution::InterParameterDistribution() {
	timeSpent = 0;
	InterName = "ParameterDistribution";

	setCurrentWorkingSubection(InterName, CM_MODEL);


	//Read Data:
	enabled = 0;
	readValueToProperty("ParameterDistributionEnable",&enabled,-1, "[0,1]");

	//TODO: ERROR HANDLING!
	if(enabled || world->bGiveKeys) {

		//Find how many scripts there are:
		nScripts = 1;
		//readValueToProperty("ParameterDistributionNSchemes",&nScripts,-2);

		if(world->bGiveKeys) {

			printToKeyFile("  ~#ParameterDistribution#~\n");

			char szKey[_N_MAX_STATIC_BUFF_LEN];
			char szParameterDistributionMode[_N_MAX_STATIC_BUFF_LEN];

			sprintf(szKey, "ParameterDistribution_%d_Mode", 0);
			sprintf(szParameterDistributionMode, "Uniform");
			readValueToProperty(szKey, szParameterDistributionMode, 2, "{Uniform, Posterior}");

			//Instantiate and explain:
			ParameterDistribution *testScript = new ParameterDistributionUniform();

			testScript->init(0,world);

			delete testScript;

			
			testScript = new ParameterDistributionPosterior();

			testScript->init(0,world);

			delete testScript;

		} else {

			//Allocate scripts
			scripts.resize(nScripts);

			char szKey[_N_MAX_STATIC_BUFF_LEN];
			char szParameterDistributionMode[_N_MAX_STATIC_BUFF_LEN];

			for(int iScript=0; iScript<nScripts; iScript++) {

				sprintf(szKey, "ParameterDistribution_%d_Mode", iScript);
				sprintf(szParameterDistributionMode, "Uniform");
				readValueToProperty(szKey, szParameterDistributionMode, 2, "s{Uniform, Posterior}");

				makeUpper(szParameterDistributionMode);
				switch(szParameterDistributionMode[0]) {
				case 'U':
					scripts[iScript] = new ParameterDistributionUniform();
					break;
				case 'P':
					scripts[iScript] = new ParameterDistributionPosterior();
					break;
				default:
					reportReadError("ERROR: Unrecognised Parameter Distribution mode: %s\n",szParameterDistributionMode);
					break;
				}
				
				scripts[iScript]->init(iScript, world);

			}

		}

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}


	//Set up intervention:
	if(enabled && !world->bGiveKeys) {

		frequency = 1e30;
		timeFirst = world->timeStart;
		timeNext = timeFirst;

	}

}

InterParameterDistribution::~InterParameterDistribution() {

	if (scripts.size() != 0) {
		for (int iScript = 0; iScript < nScripts; iScript++) {
			delete scripts[iScript];
		}
	}

}

int InterParameterDistribution::intervene() {
	//printf("PARAMETERDISTRIBUTIONS!\n");

	for(int iCurrentScript=0; iCurrentScript<nScripts; iCurrentScript++) {
		scripts[iCurrentScript]->sample();
	}

	//Update time of next event:
	timeNext += frequency;

	return 1;
}

int InterParameterDistribution::reset() {

	if(enabled) {

		timeNext = timeFirst;

	}

	return enabled;
}

int InterParameterDistribution::finalAction() {
	//printf("InterParameterDistribution::finalAction()\n");

	return 1;
}

void InterParameterDistribution::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Ok\n");
		fprintf(pFile, "Managed %d scripts\n\n", nScripts);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
