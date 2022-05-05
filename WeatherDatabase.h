//WeatherDatabase.h

#pragma once

#include "WeatherModifiers.h"
#include "classes.h"

class WeatherDatabase {

public:

	WeatherDatabase();
	~WeatherDatabase();

	int init(Landscape *pWorld);

	void addFileToQueue(string sFileName);

	void applyFileIDtoProperty(string sID, vector<PopModPair> &vTargets);

protected:

	Landscape *world;

	map<string, Raster<double> > mapIDtoRasters;

};
