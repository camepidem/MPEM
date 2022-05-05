//EventManager.h

#pragma once

#include "classes.h"

//TODO: Implement this, although probably as a SimulationManager instead

class EventManager {

public:

	EventManager();
	~EventManager();

	int setLandscape(Landscape *myWorld);

	int advanceSimulation(double dTimeNextInterrupt);

protected:

	Landscape *world;

};
