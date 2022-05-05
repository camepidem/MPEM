//InterBatch.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "InterEndSim.h"
#include "InterOutputDataDPC.h"
#include "InterBatch.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

//TODO: Make sure all interventions are batch compliant
InterBatch::InterBatch() {
	timeSpent = 0;
	InterName = "Batch";

	setCurrentWorkingSubection(InterName, CM_ENSEMBLE);

	frequency = 1e30;
	timeFirst = world->timeEnd + frequency;
	timeNext = timeFirst;

	bBatchDone = true;

	int tempInt = 0;
	enabled = 0;
	readValueToProperty("BatchEnable", &enabled,-1, "[0,1]");

	if(enabled) {
		
		//Read Data:
		nTotal = 1;
		readValueToProperty("BatchRuns",&nTotal,2, ">0");

		nMaxDurationSeconds = 0;
		readValueToProperty("BatchMaxDurationSeconds", &nMaxDurationSeconds, -2, ">=0");

		if (nMaxDurationSeconds < 0) {
			reportReadError("ERROR: Batch max duration in seconds was negative, read: %d", nMaxDurationSeconds);
		}

	}

	//Set up intervention:
	if(enabled && !world->bGiveKeys) {

		frequency = world->timeEnd - world->timeStart;
		timeFirst = world->timeEnd;
		timeNext = timeFirst;

		bBatchDone = false;

		//Prevent simulations ending prematurely:
		world->interventionManager->pInterEndSim->disableEndAtTimeCondition();

		//Stop DPC spamming to the screen:
		world->interventionManager->pInterDPC->makeQuiet();

		//Iteration counters:
		nDone = 0;
		pctDone = 0;

		//Duration limit
		nTimeStart = clock_ms();

		//Progress reporting:
		nTimeLastOutput = clock_ms();

		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char *timestring = asctime(timeinfo);

		char tempFileName[_N_MAX_STATIC_BUFF_LEN];
		sprintf(tempFileName,"%sBatchProgress.txt",szPrefixOutput());
		progressFileName = string(tempFileName);

		FILE *pBatchProgressFile;
		pBatchProgressFile = fopen(progressFileName.c_str(),"w");
		if(pBatchProgressFile) {
			fprintf(pBatchProgressFile,"Batch Started: %s\n",timestring);
			fclose(pBatchProgressFile);
		} else {
			printk("\nERROR: Risk Map Progress unable to write file\n\n");
		}

	}
}

InterBatch::~InterBatch() {

}

int InterBatch::intervene() {
	//printf("Batch!\n");

	//Update number complete:
	nDone++;

	bool bTimeLimitExceeded = nMaxDurationSeconds > 0 && clock_ms() > (nMaxDurationSeconds * 1000);

	if (nDone < nTotal && !bTimeLimitExceeded) {

		//Force creation of summary data:
		world->interventionManager->pInterEndSim->performFinalAction();

		//Force final data outputs:
		//Fiddle times in case we're being ended early
		//TODO: One day(!) make all this nice by hoisting this logic up to the actual simulation, rather than being replicated and controlled inside interventions
		auto actualTimeEnd = world->timeEnd;
		world->timeEnd = world->time;
		for(int iInter=world->interventionManager->iInterDataExtractionStart; iInter <= world->interventionManager->iInterDataExtractionEnd; iInter++) {
			world->interventionManager->interventions[iInter]->performFinalAction();
		}
		world->timeEnd = actualTimeEnd;

		//clean landscape using touched list ala R0Map
		world->cleanWorld(0,1,1);

		//Flag landscape needs to do full recalculation:
		world->bFlagNeedsFullRecalculation();

		//Reset all other interventions:
		world->interventionManager->resetInterventions();

	}

	//Progress reporting:
	double proportionDone = double(nDone) / double(nTotal);
	int newPct = int(100.0*proportionDone);
	int tNow = clock_ms();

	if (newPct > pctDone || (tNow - nTimeLastOutput) > 60000) {

		if (newPct > pctDone) {
			pctDone = newPct;
		}

		double tElapsedMiliSeconds = tNow - nTimeStart;

		int tRemainingSeconds = 1000;
		if (proportionDone > 0.0) {
			tRemainingSeconds = int(ceil(0.001 * tElapsedMiliSeconds * (1.0 - proportionDone) / (proportionDone)));
		}

		double averageTimePerRun = 0.001 * tElapsedMiliSeconds / double(nDone);

		//May not have reached a new percentage marker, but if more than 1 minute has elapsed since last progress output, give some information:
		printToFileAndScreen(progressFileName.c_str(), 1, "Runs: %d done of %d total. Completion: %.2f%% Average time per run(s): %.2f Elapsed(s): %d ETR(s): %d\n", nDone, nTotal, 100.0*proportionDone, averageTimePerRun, int(0.001*tElapsedMiliSeconds), tRemainingSeconds);

		nTimeLastOutput = tNow;
	}

	if(nDone >= nTotal || bTimeLimitExceeded) {

		bBatchDone = true;

		//force end of simulation...
		world->interventionManager->normalTerminateSimulation();
		printk("Batch Simulation Ended");
		if (bTimeLimitExceeded) {
			printk(" by exceeding time limit of %ds", nMaxDurationSeconds);
		}
		printk("\n");
	}

	return 1;
}

int InterBatch::reset() {

	return enabled;

}

int InterBatch::finalAction() {
	//printf("InterBatch::finalAction()\n");

	if(enabled) {

		//Correct the world end time:
		//world->timeEnd = frequency;
		
	}

	return 1;
}

void InterBatch::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Created Output\n");
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
