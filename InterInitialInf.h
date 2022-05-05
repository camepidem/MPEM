//InterInitialInf.h

//Contains the data for the Initial Infection Intervention

#ifndef INTERINITIALINF_H
#define INTERINITIALINF_H 1

#include "Intervention.h"

class InterInitialInf : public Intervention {

public:

	InterInitialInf();

	~InterInitialInf();

	int intervene();

	int finalAction();

	int nLocations;

	static const int WEIGHT_HOSTDENSITY						= 0;
	static const int WEIGHT_HOSTPRESENCE					= 1;
	static const int WEIGHT_HOSTDENSITY_x_SUSCEPTIBILITY	= 2;
	static const int WEIGHT_BYRASTER						= 3;

	int weightingChoice;

	void writeSummaryData(FILE *pFile);

protected:

	int nWeighted;			//Total number of locations that available to infect
	vector<double> weightings;		//Weightings of locations
	vector<int> locationIndex;		//Indices of locations

	//Stores indices of locations selected to infect
	vector<int> pSampleLocations;

};

#endif
