//InterCohortTransition.h

#pragma once

#include "Intervention.h"

class InterCohortTransition :	public Intervention
{
public:
	InterCohortTransition();
	~InterCohortTransition();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

	int reset();

protected:
	int nSchemes;
	vector<CohortTransitionScheme *> schemes;

	int iCurrentScheme;
	int getGlobalNextTime();
};

