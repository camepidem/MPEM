//InterR0Map.h

//Contains the data for the R0Map Intervention

#ifndef INTERR0MAP_H
#define INTERR0MAP_H 1

#include "Intervention.h"

class InterR0Map : public Intervention {

public:

	InterR0Map();

	~InterR0Map();

	int intervene();

	int finalAction();

	vector<vector<double>> pdSusStart;//[iWeight][iPop]
	vector<vector<double>> pnPlaceSusStart;//populations: 0...nPops-1, then Locations

	LocationMultiHost *currentLoc;

	int iStartingPopulation;

	vector<vector<vector<vector<double>>>> nLocsArray;//[weighting][snapshot][pop][loc]
	vector<vector<vector<vector<double>>>> nHostsArray;

	int bGiveVariances;
	int bGiveMeanSquare;		//Should output just expectation of squares (easier to combine variance of runs)

	vector<vector<vector<vector<double>>>> nVarLocsArray;
	vector<vector<vector<vector<double>>>> nVarHostsArray;

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

	//Maximum duration limit:
	int nMaxDurationSeconds;//TODO: Can't do this due to the way R0Map works, outputting lines at a time: Now that we live in the future and RAM is plentiful, can go back to storing the whole thing
	int nTimeStart;//TODO: clock functions currently used are in ms, and so will exceed signed 32bit int representation in ~1 month

	//Progress
	int currentIteration;

	int nTotal;
	int nDone;
	double dTotal;
	double dDone;

	int pctDone;

	int nTimeLastOutput;

	string progressFileName;

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
