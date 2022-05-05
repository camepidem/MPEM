//InterThreatMap.h

//Contains the data for the ThreatMap Intervention

#ifndef INTERTHREATMAP_H
#define INTERTHREATMAP_H 1

#include "Intervention.h"

class InterThreatMap : public Intervention {

public:

	InterThreatMap();

	~InterThreatMap();

	int intervene();

	int finalAction();

	double **pdSusStart;//[iWeight][iPop]
	double **pnPlaceSusStart;//populations: 0...nPops-1, then Locations

	LocationMultiHost *currentLoc;

	int iStartingPopulation;

	double ****nLocsArray;//[weighting][snapshot][pop][loc]
	double ****nHostsArray;

	int bGiveVariances;
	int bGiveMeanSquare;		//Should output just expectation of squares (easier to combine variance of runs)

	double ****nVarLocsArray;
	double ****nVarHostsArray;

	int infectCurrentLoc();

	int bUseUniformBackgroundInoculation;
	double dUniformBackgroundRate;
	double dWaitingTime;

	//Multiple result snapshots:
	double outputFrequency;
	int nOutputs;
	int nSnapshot;

	int fixedStart;
	int nInfect;
	int xPos;
	int yPos;

	int nWeightings;
	
	int windowXmin, windowXmax, windowYmin, windowYmax;

	//Progress
	int currentIteration;

	int nTotal;
	int nDone;
	double dTotal;
	double dDone;

	int pctDone;

	int nTimeLastOutput;

	char *progressFileName;

	void writeSummaryData(FILE *pFile);

protected:

	int inBounds(LocationMultiHost *loc);

	void allocateStorage();

	void resetStorage(int iElement=-1);

	void estimateTotalCost();

	void getStartingData();

	void selectNextLocation();

	void writeFileHeaders();

	void writeDataLine();

	void writeArrayLine(char *szFileName, double *pArray);
	void writeArrayLine(char *szFileName, int *pArray);

	void writeDataFixed();

};

#endif
