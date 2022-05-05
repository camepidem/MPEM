// SimulationProfiler.h

#pragma once

#include "ProfileTimer.h"

//For now this will just aggregate the various timers and be responsible for aggregating the information for output
class SimulationProfiler {

public:

	SimulationProfiler();
	~SimulationProfiler();

	ProfileTimer overallTimer;
	ProfileTimer simulationTimer;
	ProfileTimer initialisationTimer;
	ProfileTimer interventionTimer;

	ProfileTimer fullRecalculationTimer;
	int nFullRecalculations;			//number of full recalculations done
	ProfileTimer infectionRateRecalculationTimer;
	int nInfectionRateRecalculations;			//number of infection rate recalculations done

	int nCouplingEvents;

	ProfileTimer kernelCouplingTimer;
	ProfileTimer kernelVSTimer;


	//Event profiling:
	int64_t totalNoEvents;				//total number of productive events the simulation has performed
	int nIllegalEvents;					//Number of times floating point errors have caused illegal event selection

	void writeSummaryData(char *szSummaryFileName, int64_t nKernelLongAttempts, int64_t nKernelLongSuccesses);

protected:

	vector<ProfileTimer *> timers;

	void resetTimers();

	//Time Profiling:
	int timeInter, timeSim, timeInitialise, timeTotal;//time in ms the simulation has spent performing interventions/actually simulating/reading data/total

	int timeFullRecalculation;			//Quantity of time (part of timeSim) spent having to fully recalculate event rates rather than being able to modify cached values. Distinct from infectionRateRecalculation
	int timeInfectionRateRecalculation;	//Quantity of time (part of timeSim) spent having to recalculate infection event rates rather than being able to modify cached values. Distinct from fullRateRecalculation

	int timeCoupling;
	int timeVS;

};
