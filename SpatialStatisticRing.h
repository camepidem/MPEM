#pragma once

#include "SpatialStatistic.h"

//TODO: Put all the actual logic into this class, then the can just have SpatialStatistic<T> classes that work via dependency injection
struct StatisticDataRing {

	//Statistic data:
	vector<int> posBins, negBins;

	vector<int> realAnnulaeSizes;

	//Statistic configuration
	double radiusStep;
	double radiusMax;
	int nBins;
	int centreX;
	int centreY;

};

class SpatialStatisticRing : public SpatialStatistic
{
public:
	SpatialStatisticRing();
	~SpatialStatisticRing();

	int setup(std::vector<TargetData*> data);
	int reset();
	std::map<std::string, double> calculateStats(double tNow, TargetData data, const Raster<int> *simulationState);
	std::vector<std::string> statsHeader();
	void setupFile();

	void getRingStats(const Raster<int> *data, int *posBins, int *negBins, const Raster<int> *mask=NULL);
	
	double dPercentileIncrement;

	int bOutputFullStats;

	StatisticDataRing statConfig;

	std::map<std::string, StatisticDataRing> dataStats;

};

