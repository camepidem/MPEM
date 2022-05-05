//InterOutputDataDPC.h

//Contains the data for the Output Data Intervention

#ifndef INTEROUTPUTDATADPC_H
#define INTEROUTPUTDATADPC_H 1

#include "Intervention.h"

class InterOutputDataDPC : public Intervention {

public:

	InterOutputDataDPC();

	~InterOutputDataDPC();

	//string fName;
	int nWeightings;
	int nZonings;
	int nZones;
	vector<FILE *> pDPCFiles;
	vector<vector<FILE *>> pZoningDPCFiles;

	int bSampleKernelStatistics;
	double dTimeLastOutput;
	
	int nKernelQuantities;
	vector<vector<double>> pdKernelQuantities;
	
	vector<FILE *> pKernelDataFiles;

	int makeQuiet();

	int bQuiet;

	void displayHeaderLine();

	void writeSummaryData(FILE *pFile);

	double getFrequency();

	int bBufferDPCOutput;

protected:

	int intervene();
	int reset();
	int finalAction();

	char *szLineBuffer;

	int setupOutput();
	int clearMemory();

	int getDPCHeader();

	int getDPCLine(double dTimeNow, int iWeighting = 0, int iZoning = -1, int iZone = -1);

	int writeKernelStats(double dTimeNow);

};

#endif
