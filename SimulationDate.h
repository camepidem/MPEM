// SimulationDate.h

#pragma once

#include "cfgutils.h"

class SimulationDate {

public:

	SimulationDate();
	SimulationDate(double date);
	SimulationDate(std::string date);
	~SimulationDate();

	SimulationDate getDate();
	void setDate(SimulationDate date);

	double getDoubleDate();

protected:

	double date;

};
