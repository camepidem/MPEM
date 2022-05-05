//ParameterDistribution.h

#pragma once

class ParameterDistribution {

public:

	ParameterDistribution();
	virtual ~ParameterDistribution();

	virtual int init(int iDist, Landscape *pWorld);

	virtual int sample();

protected:

	Landscape *world;

	int iDistribution;

};
