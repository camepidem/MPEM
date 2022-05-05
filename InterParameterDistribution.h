//InterParameterDistribution.h

//Contains the data for the Climate Intervention

#pragma once

#include "Intervention.h"

class InterParameterDistribution : public Intervention {

public:

	InterParameterDistribution();

	~InterParameterDistribution();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

	int reset();

protected:

	int nScripts;
	vector<ParameterDistribution *> scripts;

};
