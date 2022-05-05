#pragma once

#include "SpatialStatistic.h"

struct StatisticDataMoranI {

	//Statistic data:
	vector<double> moranIToRadius;

	//Statistic configuration
	double radiusStep;
	double radiusMax;
	int nBins;

};

class SpatialStatisticMoranI : public SpatialStatistic {

public:

	SpatialStatisticMoranI();
	~SpatialStatisticMoranI();

	int setup(std::vector<TargetData*> data);
	int reset();
	std::map<std::string, double> calculateStats(double tNow, TargetData data, const Raster<int> *simulationState);
	std::vector<std::string> statsHeader();
	void setupFile();

	void getMoranIStats(const Raster<int> *data, double *MoranIToRadius, const Raster<int> *mask=NULL);


	int bOutputFullStats;
	
	StatisticDataMoranI statConfig;

	std::map<std::string, StatisticDataMoranI> dataStats;

};

