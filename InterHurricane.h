//InterHurricane.h

//Contains the data for the Hurricane Intervention

#ifndef INTERHURRICANE_H
#define INTERHURRICANE_H 1

#include "Intervention.h"

class InterHurricane : public Intervention {

public:

	InterHurricane();

	~InterHurricane();

	int intervene();

	int finalAction();

	double severity;

	void writeSummaryData(FILE *pFile);

protected:


};

#endif
