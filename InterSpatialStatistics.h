//InterSpatialStatistics.h

#ifndef INTERSPATIALSTATISTICS_H
#define INTERSPATIALSTATISTICS_H 1

#include "Intervention.h"
#include "SpatialStatistic.h"

class InterSpatialStatistics : public Intervention {

public:

	InterSpatialStatistics();

	~InterSpatialStatistics();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

	int reset();

	//Single target:
	TargetData *singleTargetData;
	Raster<int> *simDataRaster;

	//Multiple targets:
	std::map<double, TargetData *> targetData;

	//Using surveyed data
	std::map<double, Raster<int>> surveyEffortMaps;
	double surveyDetectionProbability;

	enum SpatialStatisticTypes { SS_COUNTS, SS_RING, SS_MORANI, SS_CLUSTER, SS_MAX = SS_CLUSTER };
	std::map<std::string, double> calculateStats(double tNow, TargetData data, InterSpatialStatistics::SpatialStatisticTypes statistic);

	enum PositiveCriteria {PC_SYMPTOMATIC, PC_SEVERITY, PC_DETECTED};
	PositiveCriteria pcPositiveCriterion;
protected:

	//int setupOutput();

	int setupStats();

	int getStats(double dTimeNow);

	//Statistics:
	vector<SpatialStatistic *> statistics;

	int nLocationStatus(int xPos, int yPos);

	double getNextTime(double dTimeNow);

};

#endif
