#pragma once

#include "SpatialStatistic.h"

struct ClusterData {

	int index;//Only useful if have kept the filled raster this corresponds to

	int nPoints;//Number of points in the cluster
	double radius;//something like the radius of cluster about the centre of mass

				  //TODO: store index of a point within the cluster so can find again in reasonable time
	int x, y;
};

struct StatisticDataCluster {

	//Statistic data:
	vector<ClusterData> clusterInfo;

	//Statistic configuration
	//
};

class SpatialStatisticCluster : public SpatialStatistic
{
public:
	SpatialStatisticCluster();
	~SpatialStatisticCluster();

	int setup(std::vector<TargetData*> data);
	int reset();
	std::map<std::string, double> calculateStats(double tNow, TargetData data, const Raster<int> *simulationState);
	std::vector<std::string> statsHeader();
	void setupFile();

	int floodfillClusters(Raster<int> *rasterToFloodFill, const Raster<int> *stateRaster, vector<int> *pClusterSizes, const Raster<int> *mask=NULL);

	std::map<std::string, StatisticDataCluster> dataStats;

	int bOutputImages;

protected:

	int floodFillRecursive(int iX, int iY, int clusterNo, Raster<int> *fillingRaster, const Raster<int> *stateRaster);

	int floodFillQueueBased(int iSourceX, int iSourceY, int clusterNo, Raster<int> *fillingRaster, const Raster<int> *stateRaster);
};

