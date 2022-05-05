//InterControlSprayPR.h

//Contains the data for the Output Data Intervention

#ifndef INTERCONTROLSPRAYPR_H
#define INTERCONTROLSPRAYPR_H 1

#include "Intervention.h"

class InterControlSprayPR : public Intervention {

public:

	InterControlSprayPR();

	~InterControlSprayPR();

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
