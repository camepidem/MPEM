#pragma once

#include "CohortTransitionScheme.h"

class CohortTransitionSchemePeriodic : public CohortTransitionScheme
{
public:

	CohortTransitionSchemePeriodic();
	~CohortTransitionSchemePeriodic();

	int init(int i_Scheme, Landscape *pWorld);

	int applyNextTransition();

	double getNextTime();

	int reset();

protected:

	double timeFirst;
	double timeNext;

	double period;

};

