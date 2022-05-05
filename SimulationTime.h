// SimulationTime.h

#pragma once

#include "cfgutils.h"

class SimulationTime {

public:

	SimulationTime();
	SimulationTime(double dTime);
	~SimulationTime();

	SimulationTime getTime();
	void setTime(SimulationTime time);

	double getDoubleTime();

protected:

	double time;

};
