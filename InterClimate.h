//InterClimate.h

//Contains the data for the Climate Intervention

#ifndef INTERCLIMATE_H
#define INTERCLIMATE_H 1

#include "Intervention.h"

class InterClimate : public Intervention {

public:

	InterClimate();

	~InterClimate();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

	int reset();

protected:

	int nScripts;
	vector<ClimateScheme *> scripts;

	int iCurrentScript;
	int getGlobalNextTime();

};

#endif
