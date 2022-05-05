//InterClimate.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "myRand.h"
#include "ClimateScheme.h"
#include "ClimateSchemeScript.h"
#include "ClimateSchemeMarkov.h"
#include "ClimateSchemePeriodic.h"
#include "InterClimate.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterClimate::InterClimate() {
	timeSpent = 0;
	InterName = "Climate";

	setCurrentWorkingSubection(InterName, CM_MODEL);

	//Read Data:

	enabled = 0;
	readValueToProperty("ClimateEnable",&enabled,-1, "[0,1]");

	//TODO: ERROR HANDLING!
	if(enabled || world->bGiveKeys) {

		//Find how many scripts there are:
		nScripts = 1;
		readValueToProperty("ClimateNSchemes",&nScripts,-2, ">0");

		if(world->bGiveKeys) {

			printToKeyFile("  ~#ClimateScripts#~\n");

			//Instantiate and init one of each script type:
			nScripts = 2;

			printToFileAndScreen(world->szKeyFileName,0,"\n// Example of climate script usage:\n");

			printToFileAndScreen(world->szKeyFileName,0,"//  For each scheme, indexed by INDEX from 0 to (ClimateNSchemes-1), supply \n//  ClimateScheme_INDEX_Mode MODE\n//   and associated keys\n");
			printToFileAndScreen(world->szKeyFileName,0,"//  Where availables MODE is from:\n\n");
			
			printToKeyFile("//  ClimateScheme_~i[0,#ClimateNSchemes#)~_Mode Script ~s{Script, Markov, Periodic}~\n");

			ClimateScheme *testScript;


			printToFileAndScreen(world->szKeyFileName,0,"// Climate - Script\n");
			printToKeyFile("// ClimateScheme_INDEX_Mode S\n");
			
			testScript = new ClimateSchemeScript();

			testScript->init(0,world);

			delete testScript;


			printToFileAndScreen(world->szKeyFileName,0,"// Climate - Markov\n");
			printToKeyFile("// ClimateScheme_INDEX_Mode M\n");

			testScript = new ClimateSchemeMarkov();

			testScript->init(0,world);

			delete testScript;
			

			printToFileAndScreen(world->szKeyFileName, 0, "// Climate - Periodic\n");
			printToKeyFile("// ClimateScheme_INDEX_Mode P\n");

			testScript = new ClimateSchemePeriodic();

			testScript->init(0, world);

			delete testScript;

		} else {

			//Allocate scripts
			scripts.resize(nScripts);

			char szKey[_N_MAX_STATIC_BUFF_LEN];
			char szClimateMode[_N_MAX_STATIC_BUFF_LEN];

			for (int iScript = 0; iScript < nScripts; iScript++) {

				sprintf(szKey, "ClimateScheme_%d_Mode", iScript);
				sprintf(szClimateMode, "Script");
				readValueToProperty(szKey, szClimateMode, 2, "{Script, Markov}");

				makeUpper(szClimateMode);
				switch(szClimateMode[0]) {
				case 'S':
					scripts[iScript] = new ClimateSchemeScript();
					break;
				case 'M':
					scripts[iScript] = new ClimateSchemeMarkov();
					break;
				case 'P':
					scripts[iScript] = new ClimateSchemePeriodic();
					break;
				default:
					reportReadError("ERROR: Unrecognised climate mode: %s\n",szClimateMode);
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

		getGlobalNextTime();

	}

}

InterClimate::~InterClimate() {

	if (scripts.size() != 0) {
		for (int iScript = 0; iScript < nScripts; iScript++) {
			delete scripts[iScript];
		}
	}

}

int InterClimate::intervene() {
	//printf("CLIMATE!\n");

	//Ensure have the right next event:
	getGlobalNextTime();

	if(iCurrentScript >=0) {
		scripts[iCurrentScript]->applyNextTransition();
	}

	//Update time of next event:
	getGlobalNextTime();

	return 1;
}

int InterClimate::reset() {

	if(enabled) {

		for(int iScript=0; iScript<nScripts; iScript++) {
			scripts[iScript]->reset();
		}

		getGlobalNextTime();

	}

	return enabled;
}

int InterClimate::getGlobalNextTime() {

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

int InterClimate::finalAction() {
	//printf("InterClimate::finalAction()\n");

	return 1;
}

void InterClimate::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Ok\n");
		fprintf(pFile, "Managed %d scripts\n\n", nScripts);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
