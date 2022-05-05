// InterventionManager.h
//
//

#pragma once

#include "classes.h"

class InterventionManager {

public:

	//Constructor/destructor:
	InterventionManager();
	~InterventionManager();

	static int setLandscape(Landscape *myWorld);

	int init();

	int checkInitialDataRequired();

	int preSimulationInterventions();

	int writeSummaryData(char * fName);

	int abortSimulation();

	int normalTerminateSimulation();

	//TODO: Instead of current indexing, have set of iterators
	vector<Intervention *> interventions;//array of possible interventions
	InterEndSim *pInterEndSim;			//Index of the primary end simulation intervention
	int iInterDataExtractionStart;		//Index of first data extraction intervention
	InterOutputDataDPC *pInterDPC;		//Index of DPC data output intervention
	InterOutputDataRaster *pInterRaster;//Index of Raster data output intervention
	InterSpatialStatistics *pInterSpatialStatistics;//Index of Spatial Statistics intervention
	InterVideo *pInterVideo;			//Index of Video data output intervention
	int iInterDataExtractionEnd;		//
	int iInterEnvironmentalStart;		//Index of first data Environmental intervention
	int iInterEnvironmentalEnd;			//
	int iInterControlStart;				//Index of first control intervention
	int iInterControlEnd;				//
	int iInterSimulationStateStart;		//Index of first Simulation State intervention
	int iInterSimulationStateEnd;		//
	int iInterBulkModeStart;			//Index of first data Bulk Mode intervention
	InterRiskMap	*pInterRiskMap;		//Index of risk map intervention
	InterR0Map		*pInterR0Map;		//Index of R0 map intervention
	InterThreatMap	*pInterThreatMap;	//Index of threat intervention
	InterBatch		*pInterBatch;		//Index of Batch intervention
	int iInterBulkModeEnd;				//

	int performIntervention();			//executes required intervention
	int manageInterventions();			//manages interventions
	int finalInterventions();			//Executes each interventions final action
	int nextIntervention;				//index of next intervention to occur
	double timeNextIntervention;		//time at which next intervention will occur

	int requestDisableAllInterventionsExcept(int nInter, char *szInterName, double dMaxTime);	//An intervention, index nInter, requests all other interventions be disabled for the duration of its run
	int disableStandardInterventions(int nInter, double);			//Stops standard stopping condition and DPC and raster data output - part of RiskMap while control is enabled.
	int nRemainingIntervention;										//Number of intervention that will not be disabled
	
	int requestDisableStandardInterventions(Intervention *pInter, char *szInterName, double dMaxTime);//Request to only disable standard data outputting interventions
	int bDisableStandardOnly;
	double dDisableTime;
	int disableInterventions();
	int disableAllInterventionsExcept();							//Disables all interventions except the one specified by index nRemainingIntervention - used to override other interventions when using R0Map or RiskMap
	int disableStandardInterventions();
	int resetInterventions();										//Resets all interventions to their time=0.0 state, used with InterRiskMap



protected:

	static Landscape *world;

};
