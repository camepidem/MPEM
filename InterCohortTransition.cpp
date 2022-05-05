//InterCohortTransition.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "myRand.h"
#include "CohortTransitionScheme.h"
#include "CohortTransitionSchemeScript.h"
#include "CohortTransitionSchemePeriodic.h"
#include "InterCohortTransition.h"

#pragma warning(disable : 4996)		/* stop Visual C++ from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterCohortTransition::InterCohortTransition() {
	timeSpent = 0;
	InterName = "CohortTransition";

	setCurrentWorkingSubection(InterName, CM_MODEL);

	//Read Data:

	enabled = 0;
	readValueToProperty("CohortTransitionEnable", &enabled, -1, "[0,1]");

	//TODO: ERROR HANDLING!
	if (enabled || world->bGiveKeys) {

		//Find how many schemes there are:
		nSchemes = 1;
		readValueToProperty("CohortTransitionNSchemes", &nSchemes, -2, ">0");

		if (world->bGiveKeys) {

			printToKeyFile("  ~#CohortTransitionScripts#~\n");

			//Instantiate and init one of each scheme type:
			nSchemes = 2;

			printToFileAndScreen(world->szKeyFileName, 0, "\n// Example of CohortTransition script usage:\n");

			printToFileAndScreen(world->szKeyFileName, 0, "//  For each scheme, indexed by INDEX from 0 to (CohortTransitionNSchemes-1), supply \n//  CohortTransitionScheme_INDEX_Mode MODE\n//   and associated keys\n");
			printToFileAndScreen(world->szKeyFileName, 0, "//  Where availables MODE is from:\n\n");

			printToKeyFile("//  CohortTransitionScheme_~i[0,#CohortTransitionNSchemes#)~_Mode Script ~s{Script, Periodic}~\n");

			CohortTransitionScheme *testScript;

			printToFileAndScreen(world->szKeyFileName, 0, "// CohortTransition - Script\n");
			printToKeyFile("// CohortTransitionScheme_INDEX_Mode S\n");

			testScript = new CohortTransitionSchemeScript();

			testScript->init(0, world);

			delete testScript;


			printToFileAndScreen(world->szKeyFileName, 0, "// CohortTransition - Periodic\n");
			printToKeyFile("// CohortTransitionScheme_INDEX_Mode P\n");

			testScript = new CohortTransitionSchemePeriodic();

			testScript->init(0, world);

			delete testScript;


		} else {

			//Allocate schemes
			schemes.resize(nSchemes);

			char szKey[_N_MAX_STATIC_BUFF_LEN];
			char szCohortTransitionMode[_N_MAX_STATIC_BUFF_LEN];

			for (int iScript = 0; iScript < nSchemes; iScript++) {

				sprintf(szKey, "CohortTransitionScheme_%d_Mode", iScript);
				sprintf(szCohortTransitionMode, "Script");
				readValueToProperty(szKey, szCohortTransitionMode, 2, "{Script, Periodic}");

				makeUpper(szCohortTransitionMode);
				switch (szCohortTransitionMode[0]) {
				case 'S':
					schemes[iScript] = new CohortTransitionSchemeScript();
					break;
				case 'P':
					schemes[iScript] = new CohortTransitionSchemePeriodic();
					break;
				default:
					reportReadError("ERROR: Unrecognised CohortTransition mode: %s\n", szCohortTransitionMode);
					break;
				}

				schemes[iScript]->init(iScript, world);

			}

		}

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}


	//Set up intervention:
	if (enabled && !world->bGiveKeys) {

		getGlobalNextTime();

	}

}


InterCohortTransition::~InterCohortTransition()
{

	if (schemes.size() != 0) {
		for (int iScript = 0; iScript < nSchemes; iScript++) {
			delete schemes[iScript];
		}
	}

}

int InterCohortTransition::intervene() {
	//printf("CohortTransition!\n");

	//Ensure have the right next event:
	getGlobalNextTime();

	if (iCurrentScheme >= 0) {
		schemes[iCurrentScheme]->applyNextTransition();
	}

	//std::cerr << "CohortTransition" << std::endl;
	//world->bForceDebugFullRecalculation = 1;

	//Update time of next event:
	getGlobalNextTime();

	return 1;
}

int InterCohortTransition::reset() {

	if (enabled) {

		for (int iScheme = 0; iScheme<nSchemes; iScheme++) {
			schemes[iScheme]->reset();
		}

		getGlobalNextTime();

	}

	return enabled;
}

int InterCohortTransition::getGlobalNextTime() {

	double dMinTime = world->timeEnd + 1e30;

	for (int iScheme = 0; iScheme<nSchemes; iScheme++) {

		double dNextTime = schemes[iScheme]->getNextTime();
		if (dNextTime < dMinTime) {
			dMinTime = dNextTime;
			iCurrentScheme = iScheme;
		}

	}

	timeNext = dMinTime;

	return 1;

}

int InterCohortTransition::finalAction() {
	//printf("InterCohortTransition::finalAction()\n");

	return 1;
}

void InterCohortTransition::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n", InterName);

	if (enabled) {
		fprintf(pFile, "Ok\n");
		fprintf(pFile, "Managed %d schemes\n\n", nSchemes);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}

