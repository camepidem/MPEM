//InterControlSprayAS.h

//Contains the data for the Output Data Intervention

#ifndef INTERCONTROLSPRAYAS_H
#define INTERCONTROLSPRAYAS_H 1

#include "Intervention.h"

class InterControlSprayAS : public Intervention {

public:

	InterControlSprayAS();

	~InterControlSprayAS();

	int intervene();

	int finalAction();

	double cost;

	double totalCost;

	double radius;

	double effect;

	int iPopulationToControl;

	int bOuputStatus;

	void writeSummaryData(FILE *pFile);

protected:


};

#endif
