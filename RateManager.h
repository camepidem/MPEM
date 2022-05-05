//RateManager.h

#ifndef RATEMANAGER_H
#define RATEMANAGER_H 1

#include "classes.h"
#include "RateConstantHistory.h"

enum EventSources {EVENTSOURCE_LANDSCAPE, EVENTSOURCE_EXTERNAL, EVENTSOURCE_LENGTH};

struct NextEventDataStruct {

	EventSources ES_Source;

	//EVENTSOURCE_LANDSCAPE
	int iLastEventPopType;
	int iLastEventNo;
	int iLastLocation;

	//EVENTSOURCE_EXTERNAL
	int iExternalEvent;

};

class RateManager {

public:

	//Constructor/destructor:
	RateManager();
	~RateManager();

	//Interface methods:
	static int setLandscape(Landscape *myWorld);
	int readRateConstants();

	int scaleRateConstantFromDefault(int iPop, int iEventType, double factor);
	int changeRateConstantDefault(int iPop, int iEventType, double dNewRateConstant);

	int refreshSporulationRateConstants();

	static int getPopAndEventFromRateConstantName(char *szConstName, int &iPop, int &iEventType);
	static int getRateConstantNameFromPopAndEvent(int iPop, int iEventType, char *szConstName);

	double getRateConstantAverage(int iPop, int iEventType, double dTimeStart, double dTimeEnd);

	//Rate reporting:
	double submitRate(int iPop, int locNo, int iEventType, double rate);
	double submitRate_dumb(int iPop, int locNo, int iEventType, double rate);
	double getRate(int iPop, int locNo, int iEventType);

	void infRateScrub(); //Zero all infection process related rates: Used in the InfectionRateRecalculation to zero all rates and then selectively recalculate only the non zero ones

	void infRateResum(); //Resum only those rates that come from infection related events: infection and virtual sporulation
	void fullResum(); //Resum all rates
	
	int requestExternalRate(Intervention *requestingIntervention, double dInitialRate=0.0, int iAdditionalArgument=0);
	double submitRateExternal(int iExternalEvent, double rate);

	//Event selection:
	double selectNextEvent();
	int executeNextEvent();


	int scrubRates(int bCleanAllRates);												//cleans out all rates (part of interventions)

	//TODO: Calculate operations more precisely for different types
	double getWorkToResubmit();

	//Flag showing if Interval structure is being used:
	//TODO: Split this on a per population basis...
	enum RateStructureType {AUTO, SUM, INTERVALS, TREE, COMPOSITIONREJECTION, RATESTRUCTURE_MAX};
	static const char *RateStructureTypeNames[];

	static RateStructureType rsRateStructureToUseForRateSLoss;
	static RateStructureType rsRateStructureToUseForStandards;

	static void writeSummaryData(FILE *pSummaryFile);

	static int bStandardSpores;

protected:

	static Landscape *world;

	//Array of rate structures for each event:
	//TODO: Have RateStructures for RateStructures? - avoids having to manually select events at the structure level...
	vector<vector<RateStructure *>> eventRates;

	//Rate constants for each event:
	static vector<vector<double>> pdRateConstantsEffective;
	static vector<vector<double>> pdRateConstantsScaleFactors;
	static vector<vector<double>> pdRateConstantsDefault;
	
	vector<vector<RateConstantHistory>> rateConstantHistories;

	//Non location related events:
	RateStructure *eventRatesExternal;
	vector<int> piAdditionalArguments;
	vector<Intervention *> externalRateInterventions;
	int nEventsExternal;
	int nMaxEventsExternal;//Don't know ahead of time how many to allocate, so allocate this many and then grow as necessary

	//Event selection:
	NextEventDataStruct nextEventData;//Stores information on next event selected to occur
	double getTotalRateLandscape();
	double getTotalRateExternal();
	double getTotalRate();
	double getTotalPopulationRate(int iPop);
	double getTotalEventRate(int iPop, int iEventType);
	int getPopulationEventIndex(int iPop, int iEventType, double rate);

	static double getEffectiveRateConstantSLoss(int iPop, double dNewBeta);
	static double getEffectiveRateConstantVirtualSporulation(int iPop, double dNewBeta);

};



#endif
