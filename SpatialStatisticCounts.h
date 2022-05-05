#pragma once

#include "SpatialStatistic.h"

struct StatisticDataCount {

	//Statistic data:
	int nDataPos;
	int nDataNeg;

	//Statistic configuration
	//
};

class SpatialStatisticCounts : public SpatialStatistic
{
public:
	SpatialStatisticCounts();
	~SpatialStatisticCounts();

	int setup(std::vector<TargetData*> data);
	int reset();
	std::map<std::string, double> calculateStats(double tNow, TargetData data, const Raster<int> *simulationState);
	std::vector<std::string> statsHeader();
	void setupFile();

	void getCounts(const Raster<int> *data, const Raster<int> *simulationState, int &nPos, int &nNeg, int &nTruePos, int &nTrueNeg, int &nFalsePos, int &nFalseNeg, const Raster<int> *mask=NULL);

	//Don't need to cache anything as all information is calculated in the normal process
};
