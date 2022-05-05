#pragma once

#include "ClimateScheme.h"

class ClimateSchemePeriodic : public ClimateScheme
{
public:

	ClimateSchemePeriodic();
	~ClimateSchemePeriodic();

	int init(int i_Scheme, Landscape *pWorld);

	int applyNextTransition();

	double getNextTime();

	int reset();

protected:

	double timeFirst;
	double timeNext;

	double period;

	double duration;

	double offFactor;
	double onFactor;

	int currentState;

};

