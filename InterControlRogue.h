//InterControlRogue
	
//Contains the data for the Roguing Intervention

#ifndef INTERCONTROLROGUE_H
#define INTERCONTROLROGUE_H 1

#include "Intervention.h"

class InterControlRogue : public Intervention {

public:

	InterControlRogue();

	~InterControlRogue();

	int intervene();

	int finalAction();

	double cost;

	double totalCost;

	double radius;

	double effectiveness;

	int iPopulationToControl;

	void writeSummaryData(FILE *pFile);

protected:


};

#endif
