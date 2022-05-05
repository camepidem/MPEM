//InterRiskMap.h

//Contains the data for the R0Map Intervention

#ifndef INTERRISKMAP_H
#define INTERRISKMAP_H 1

#include "Intervention.h"

class InterRiskMap : public Intervention {

public:

	InterRiskMap();

	~InterRiskMap();

	int intervene();

	int finalAction();

	//Multiple result snapshots:
	double outputFrequency;
	int nOutputs;
	int nSnapshot;

	//Number of replicates:
	int nTotal;
	int nDone;
	
	int bGiveVariances;		//stores and variances
	int bGiveMeanSquare;	//Should output just expectation of squares (easier to combine variance of runs)

	int bTrackProbInfection;
	int bTrackProbSymptomatic;
	int bTrackSeverity;
	int bTrackTimeFirstInfection;

	int bTrackInoculumExport;
	int bTrackInoculumDeposition;

	int bOutputProbInfection;//Need to track PInfection if use either MeanSev or TimeFirstInf, but may not want to output it...

	int bAllowStandardOutput;

	//Results:
	//Snapshots:
	int nElementsTotalStorage;

	int iElementsProbInfection;
	int iElementsProbSymptomatic;
	int iElementsMeanSeverity;
	int iElementsTimeFirstInfection;

	int iElementsInoculumExport;
	int iElementsInoculumDeposition;

	//Maximum duration limit:
	int nMaxDurationSeconds;
	int nTimeStart;//TODO: clock functions currently used are in ms, and so will exceed signed 32bit int representation in ~1 month

	//Progress monitoring:
	string progressFileName;
	int nTimeLastOutput;

	int pctDone;

	int iInitialRunNo;

	int reset();

	void writeSummaryData(FILE *pFile);

};

#endif
