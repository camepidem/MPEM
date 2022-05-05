// InterventionManager.cpp
// 
// 

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "SimulationProfiler.h"
#include "InterventionHeaders.h"
#include "InterventionManager.h"

#pragma warning(disable : 4996)

Landscape *InterventionManager::world;

InterventionManager::InterventionManager() {

}

InterventionManager::~InterventionManager() {

	for (size_t iInter = 0; iInter < interventions.size(); ++iInter) {
		delete interventions[iInter];
	}

}

int InterventionManager::init() {

	setCurrentWorkingSection("INTERVENTIONS");

	//TODO: Put a static member on each intervention that determines how many instances it needs?
	//Better solution is to handle multiple instancing internally

	//Allocate interventions static pointer to world:
	Intervention::setWorld(world);

	//Have not disabled any interventions yet:
	nRemainingIntervention = -1;
	bDisableStandardOnly = 0;
	dDisableTime = world->timeStart;

	//Basic Interventions:
	//
	

	//The End of Simulation Intervention
	pInterEndSim = new InterEndSim();
	interventions.push_back(pInterEndSim);

	//Data extraction:
	iInterDataExtractionStart = interventions.size();
	//Gather data from master for OutputDataDPC
	pInterDPC = new InterOutputDataDPC();
	interventions.push_back(pInterDPC);

	//Gather data from master for OutputDataRaster
	pInterRaster = new InterOutputDataRaster();
	interventions.push_back(pInterRaster);

	//Gather data from master for SpatialStatistics
	pInterSpatialStatistics = new InterSpatialStatistics();
	interventions.push_back(pInterSpatialStatistics);

	//Gather data from master for Video
	pInterVideo = new InterVideo();
	interventions.push_back(pInterVideo);

	//Last data extraction intervention:
	iInterDataExtractionEnd = interventions.size() - 1;



	//Control interventions:
	//
	iInterControlStart = interventions.size();

	//Gather data from master for Detection Sweep
	interventions.push_back(new InterDetectionSweep());

	//Gather data from master for Control Cull
	interventions.push_back(new InterControlCull());

	//Gather data from master for Control Roguing
	interventions.push_back(new InterControlRogue());

	//Gather data from master for Control Antisporulant Spray
	interventions.push_back(new InterControlSprayAS());

	//Gather data from master for Control Protectant Spray Sweep
	interventions.push_back(new InterControlSprayPR());

	interventions.push_back(new InterManagement());

	//
	iInterControlEnd = interventions.size() - 1;


	//Weather interventions:
	//
	iInterEnvironmentalStart = interventions.size();

	//Gather data from master for Climate simulation
	interventions.push_back(new InterClimate());

	//Gather data from master for Weather simulation
	interventions.push_back(new InterWeather());

	//Gather data from master for Hurricane simulation
	interventions.push_back(new InterHurricane());

	//
	iInterEnvironmentalEnd = interventions.size() - 1;


	//Simulation State interventions:
	//
	iInterSimulationStateStart = interventions.size();

	//Gather data from master for random Initial Infection simulation
	interventions.push_back(new InterInitialInf());

	//Gather data from master for Climate simulation
	interventions.push_back(new InterVariableBackgroundInfection());

	//Gather data from master for Scripted Infection simulation
	interventions.push_back(new InterCohortTransition());

	//Gather data from master for Scripted Infection simulation
	interventions.push_back(new InterScriptedInfectionData());

	//Gather data from master for Parameter distribution sampling:
	interventions.push_back(new InterParameterDistribution());

	//
	iInterSimulationStateEnd = interventions.size() - 1;


	//Bulk Processing Modes:
	//
	iInterBulkModeStart = interventions.size();

	//Gather data from master for R0 mapping:
	interventions.push_back(pInterR0Map = new InterR0Map());

	//Gather data from master for Risk mapping:
	interventions.push_back(pInterRiskMap = new InterRiskMap());

	interventions.push_back(pInterThreatMap = new InterThreatMap());

	interventions.push_back(pInterBatch = new InterBatch());

	//
	iInterBulkModeEnd = interventions.size() - 1;

	//Ensure that only one bulk processing mode is enabled:
	int nBulkEnabled = 0;
	for(int iInter = iInterBulkModeStart; iInter <= iInterBulkModeEnd; iInter++) {
		if(interventions[iInter]->enabled) {
			nBulkEnabled++;
		}
	}

	if(nBulkEnabled > 1 && !bIsKeyMode()) {
		reportReadError("ERROR: Creating Intervention Structure Failed. Multiple (%d) incompatible bulk processing modes enabled:\n", nBulkEnabled);
		for(int iInter=iInterBulkModeStart; iInter<= iInterBulkModeEnd; iInter++) {
			if(interventions[iInter]->enabled) {
				reportReadError("%s: Enabled\n", interventions[iInter]->InterName);
			}
		}
	}

	if (pInterEndSim->bEarlyStoppingConditionsEnabled()) {
		//Check that if early stopping conditions are used, then either no Batch modes are used or only Batch is used:
		if (nBulkEnabled > 0) {
			if (!pInterBatch->enabled) {
				reportReadError("ERROR: End Simulation stopping conditions can only be used with Batch mode, not Risk maps or R0Maps.\nRisk Maps may be possible in the future.\n");
			}
		}
	}


	if(nRemainingIntervention != -1) {
		disableInterventions();
	}


	if(world->bReadingErrorFlag) {
		reportReadError("ERROR: Creating Intervention Structure Failed\n");
		return 0;
	} else {
		printk("Intervention Structure Complete\n\n");
		return 1;
	}

}

int InterventionManager::setLandscape(Landscape *myWorld) {

	world = myWorld;

	return 1;
}

//TODO: Make much better when rewrite InterventionManager...
int InterventionManager::checkInitialDataRequired() {

	int tempInt, tempInt2;

	//Default: Read but do not save
	int iInitialDataRequired = Landscape::iInitialDataRead;

	tempInt = 0;
	readIntFromCfg(world->szMasterFileName,"R0MapEnable",&tempInt);/*R0MapEnabled*/
	if(tempInt) {
		iInitialDataRequired = Landscape::iInitialDataUnnecessary;
		printk("Initial Data unnecessary due to use of R0 Map\n");
	}

	tempInt = 0;
	readIntFromCfg(world->szMasterFileName,"ThreatMapEnable",&tempInt);/*ThreatMapEnabled*/
	if(tempInt) {
		iInitialDataRequired = Landscape::iInitialDataUnnecessary;
		printk("Initial Data unnecessary due to use of Threat Map\n");
	}

	tempInt = 0;
	readIntFromCfg(world->szMasterFileName,"RandomStartLocationEnable",&tempInt);/*RandomInitialInfectionEnabled*/
	if(tempInt) {
		iInitialDataRequired = Landscape::iInitialDataUnnecessary;
		printk("Initial Data unnecessary due to use of Random initial infection\n");
	}

	if (iInitialDataRequired != Landscape::iInitialDataUnnecessary) {//If we don't need the initial data for one run, then we don't need it to do multiple runs
		tempInt = 0;
		readIntFromCfg(world->szMasterFileName, "RiskMapEnable", &tempInt);/*RiskMapEnabled*/
		tempInt2 = 0;
		readIntFromCfg(world->szMasterFileName, "BatchEnable", &tempInt2);/*BatchEnabled*/
		if (tempInt || tempInt2) {
			iInitialDataRequired = Landscape::iInitialDataSave;
			printk("Initial Data required to be saved due to use of ");
			if (tempInt) {
				printk("Risk Map\n");
			} else {
				printk("Batch Mode\n");
			}
		}
	}

	return iInitialDataRequired;

}

int InterventionManager::resetInterventions() {
	for(size_t i=0; i<interventions.size(); i++) {
		interventions[i]->performReset();
	}

	return 1;
}

int InterventionManager::preSimulationInterventions() {
	//TODO: refactor pre-simulation actions to be default part of interventions

	pInterDPC->displayHeaderLine();

	return 1;

}

int InterventionManager::writeSummaryData(char * fName) {

	FILE *pSimFile = fopen(fName, "a");

	if(pSimFile) {
		fprintf(pSimFile,"Time spent performing interventions (s):\n\n");
		for(size_t i=1; i<interventions.size(); i++) {
			if(interventions[i]->enabled) {
				fprintf(pSimFile,"%-22s %8.2f\n",interventions[i]->InterName,double(interventions[i]->timeSpent/1000.0));
			}
		}

		//Print Intervention summary strings
		printHeaderText(pSimFile,"Intervention Summary Data:");

		//List Enabled interventions first:
		for(size_t i=1; i<interventions.size(); i++) {
			if(interventions[i]->enabled) {
				interventions[i]->writeSummaryData(pSimFile);
			}
		}
		fprintf(pSimFile,"\n");
		//Then List Disabled:
		for (size_t i = 1; i<interventions.size(); i++) {
			if(!interventions[i]->enabled) {
				interventions[i]->writeSummaryData(pSimFile);
			}
		}

		fclose(pSimFile);
	} else {
		printk("ERROR: InterventionManager unable to write to summary data file\n");
	}

	return 1;
}

int InterventionManager::abortSimulation() {

	timeNextIntervention = -1.0;
	for (size_t i = 0; i<interventions.size(); i++) {
		if (interventions[i] == pInterEndSim) {
			nextIntervention = i;
		}
	}
	
	//Make sure EndSim is not disabled:
	pInterEndSim->enabled = 1;
	pInterEndSim->setTimeNext(world->timeEnd + 1e30);//Make sure EndSim thinks it should finish

	return 1;
}

int InterventionManager::normalTerminateSimulation() {
	printk("Simulation Ended\n");
	world->run = 0;
	return 1;
}

//TODO: Delegate responsibility for disabling to the interventions
//->disable(double endTime)
//TODO: Have disabling be more structured than the current hack of having set to work after everything has finished
int InterventionManager::requestDisableAllInterventionsExcept(int nInter, char *szInterName, double dMaxTime) {

	if(nRemainingIntervention != -1) {
		reportReadError("ERROR: multiple interventions [%s, %s] have requested all other interventions be disabled\n", interventions[nRemainingIntervention]->InterName, szInterName);

		return 0;
	} else {
		nRemainingIntervention = nInter;

		dDisableTime = dMaxTime;

		return 1;
	}

}

int InterventionManager::disableAllInterventionsExcept() {

	size_t nInter = nRemainingIntervention;

	if(nInter >= interventions.size() || nInter < 0) {

		reportReadError("ERROR: Intervention %d unrecognised\n", nInter);

		return 0;
	}

	//Disable all other interventions
	for(size_t i=0; i<interventions.size(); i++) {
		//TODO: Undo this hack
		if(i != nInter /*&& i != iInterVideo*/) {
			interventions[i]->disable();
		}
	}

	//Set the end time to longer than the R0 monitoring time
	world->timeEnd = dDisableTime + 1e30;

	return 1;
}

int InterventionManager::requestDisableStandardInterventions(Intervention *pInter, char *szInterName, double dMaxTime) {

	if(nRemainingIntervention != -1) {

		reportReadError("ERROR: multiple interventions [%s, %s] have requested all other interventions be disabled\n", interventions[nRemainingIntervention]->InterName, szInterName);

		return 0;
	} else {

		for (size_t i = 0; i < interventions.size(); i++) {
			if (pInter == interventions[i]) {
				nRemainingIntervention = i;
			}
		}

		if (nRemainingIntervention == -1) {

			//If we didn't find it in the existing list, it must be the one we are currently pushing onto the back of the array
			nRemainingIntervention = interventions.size();
		}

		bDisableStandardOnly = 1;

		dDisableTime = dMaxTime;

		return 1;
	}

}

int InterventionManager::disableStandardInterventions() {

	//Disable standard end simulation:
	pInterEndSim->disable();

	//Disable standard Data output interventions
	for(size_t i=iInterDataExtractionStart; i <= size_t(iInterDataExtractionEnd); i++) {
		if (interventions[i] != pInterVideo) {//TODO: keep this?
			interventions[i]->disable();
		}
	}

	//Set the end time to longer than the special intervention monitoring time
	world->timeEnd = dDisableTime + 1e30;

	//Should also go through all other interventions, and if not explicitly enabled make sure that they are properly disabled
	for(size_t i=0; i<interventions.size(); i++) {
		if(!interventions[i]->enabled) {
			interventions[i]->disable();
		}
	}

	return 1;
}

int InterventionManager::disableInterventions() {

	if(bDisableStandardOnly) {
		disableStandardInterventions();
	} else {
		disableAllInterventionsExcept();
	}


	return 1;
}

int InterventionManager::performIntervention() {
	//printf("doing the intervention\n");
	interventions[nextIntervention]->performIntervention();
	return 1;
}

int InterventionManager::manageInterventions() {
	//find the next intervention which is going to occur
	int j = 0;
	double tMin = pInterEndSim->getTimeNext();
	for(size_t i=0;i<interventions.size();i++) {
		if(interventions[i]->enabled) {
			if(interventions[i]->getTimeNext() < tMin) {
				j = i;
				tMin = interventions[i]->getTimeNext();
			}
		}
	}
	//set index
	nextIntervention = j;
	//set time
	timeNextIntervention = tMin;
	return 1;
}

int InterventionManager::finalInterventions() {
	printk("Performing Final interventions\n");
	//Going backwards through the list so that all interventions perform their last actions 
	//before data is collected by the EndSim event
	for(int i=interventions.size()-1;i>=0;i--) {

		world->profiler->interventionTimer.start();
		world->profiler->overallTimer.start();

		interventions[i]->performFinalAction();

		world->profiler->interventionTimer.stop();
		world->profiler->overallTimer.stop();

	}
	return 1;
}
