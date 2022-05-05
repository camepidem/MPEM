//InterBatch.h

//Contains the data for the Batch Intervention

#ifndef INTERBATCH_H
#define INTERBATCH_H 1

#include "Intervention.h"

class InterBatch : public Intervention {

public:

	InterBatch();

	~InterBatch();

	int intervene();

	int finalAction();

	//Number of replicates:
	int nTotal;
	int nDone;

	//Maximum duration limit:
	int nMaxDurationSeconds;
	int nTimeStart;//TODO: clock functions currently used are in ms, and so will exceed signed 32bit int representation in ~1 month

	//Progress monitoring:
	string progressFileName;
	int nTimeLastOutput;

	int pctDone;

	int reset();

	void writeSummaryData(FILE *pFile);

	bool bBatchDone;

};

#endif
