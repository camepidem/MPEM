//InterScriptedInfectionData.h

//Contains the data for the Initial Infection Intervention

#ifndef INTERSCRIPTEDINFECTIONDATA_H
#define INTERSCRIPTEDINFECTIONDATA_H 1

#include "Intervention.h"

class InterScriptedInfectionData : public Intervention {

public:

	InterScriptedInfectionData();

	~InterScriptedInfectionData();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

	int reset();

protected:

	//Vector of time snapshots of alterations to make to the landscape
	int nTimeSnapshots;
	vector<double> pdTimeSnaps;
	vector<int> pnDataPoints;
	vector<vector<PointXYV<int> *>> dataSnapshots;

	int iCurrentSnapshot;

	void findNextSnapshot(int bReset=0);

};

#endif
