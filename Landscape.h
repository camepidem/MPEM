//Landscape.h

//This object holds all the data about the landscape and coordinates the simulation
//TODO: Export simulation management responsibility/authority to SimulationManager
//TODO: refactor this to a dumb LandscapeManager

#ifndef LANDSCAPE_H
#define LANDSCAPE_H 1

#include "classes.h"
#include <vector>

class Landscape {
public:

	//Constructor/destructor:
	Landscape(int RunNo);
	virtual ~Landscape();

	int init(const char *version);

	//Handle requests to abort simulation cleanly:
	int abortSimulation(int saveData = 0);
	int bAborted;
	int bSaveData;

	int runNo;
	int setRunNo(int iRunNo);

	//Initialization:
	int bInitialisationOnTrack;	//Flag indicating if the initialisation is successful

	int initUtilities(const char *version);
	int makeRNG();
	int makeLandscape();
	int initDataStructures();
	int makeEvolutionStructure();
	int makeInterventionStructure();
	int makeKernel();

	//Data reading methods
	int bReadingErrorFlag;
	int readRasters();
	int readMaster();
	
	int readDensities();
	int readHostAllocations();

	//Data checking methods
	int verifyData();
	int checkHeader(char *szCfgFile);
	int checkInitialDataRequired();					//Performs check to see if initial data rasters are required
	int iInitialDataRequired;						//Flags if initial data needed
	enum InitialDataRequired {iInitialDataUnnecessary, iInitialDataRead, iInitialDataSave};

	char *getErrorLog();

	//Global Properties:
	
	//Simulation data
	double timeStart;					//Simulation start time
	double time;						//Time since simulation started (arbitrary units, determined by units of input parameters)
	double timeEnd;						//End of time period over which simulation will run
	unsigned long RNGSeed;				//Seed for RNG used this run
	char szFullModelType[1024];			//name of type of model being used

	//Configuration
	char *szMasterFileName;				//Name of master configuration file
	int bGiveKeys;						//Option to make all objects give information on possible options in Master file
	char *szKeyFileName;				//File to output all key names
	const char *szCurrentWorkingSection;	//Name of current section of program that is running eg Interventions, Dispersal, Model parameters, etc
	const char *szCurrentWorkingSubsection;	//Name of subsection eg particular intervention, dispersal mode, specific model type parameters

	char *szPrefixLandscapeRaster;
	char szPrefixOutput[64];
	char *szBaseDensityName;
	char *szSuffixTextFile;

	//Landscape properties
	RasterHeader<int> *header;

	//Interaction properties
	DispersalManager *dispersal;
	
	//Rates:
	RateManager *rates;					//Rate manager class

	//Data:
	//Array of all locations
	std::vector<LocationMultiHost *> locationArray;			//1d Array of pointers to locations (single indexed- 0<=i<ncols, 0<=j<nrows; element i,j is locationArray[i+j*ncols])

	//Lists
	ListManager *listManager;

	//Summary Data:
	SummaryDataManager *summaryData;

	//Evolution methods:
	int prepareLandscape();				//gathers all rates from locations
	int advanceLandscape(double dTimeNextInterrupt);//finds next event, moves time forward and executes it

	int bFlagNeedsFullRecalculation();
	int bFlagNeedsInfectionRateRecalculation();

	int bTrackingInoculumDeposition();		//Returns inoculum deposition tracking status
	int bTrackingInoculumExport();			//Returns inoculum export tracking status

	std::vector<EventInfo> lastEvents;		//events requiring resolution
	std::vector<EventInfo> recalculationQueue;//populations requiring infection rate recalculations

	//Rate handling:
	std::vector<std::vector<LocationMultiHost *>>	EventLocations;

	int simulate();						//master function starts and controls simulation
	int simulateUntilTime(double dPauseTime);//Additional interface to run simulation in chunks, not compatible with multiple runs
	int simulationFinish();				//If simulating using the untilTime interface, simulation will not complete until this function is called
	int run;							//flag determines if should stop simulation
	int breakEventLoop;

	double cleanWorld(int bForceTotalClean, int bPreventAllRateResubmission=0, int bIncrementRunNumber=0);//returns world to totally susceptible state, optionally using just Touched list, returns total quantity of susceptibles
	int resetWorld();					//returns world to starting state

	//Intervention properties
	InterventionManager *interventionManager;
	
	
	//Profiling
	SimulationProfiler *profiler;
	

	//RNG Access:
	double getSimRandom();
	double getNonSimRandom();

	double getMinimumHostScaling();
	
	int getUniqueActionIndex();

	int bForceDebugFullRecalculation;

	static int debugCompareStateToDPCs();

protected:

	//Random number generators:


	//Utility methods
	double getMinimumHostScaling(int iPop);

	//Internal flags:
	int bNeedsFullRecalculation;			//All rates need to be recalculated
	int bNeedsInfectionRateRecalculation;	//Infection rates have been invalidated and need to be recalculated
	
	int bTrackInoculumDeposition;			//Whether or not to track inoculum deposition: very costly, as need to do a lot of calculations that would not be necessary for the epidemic if tracking is not required
	int bTrackInoculumExport;				//Whether or not to track inoculum export: probably not very costly, but included for symmetry and checking how costly

	//Used to mark locations as having been acted upon in the current round of an action
	int uniqueActionIndex;

};

#endif
