//SpatialStatistic.cpp

#include "stdafx.h"
#include "SpatialStatistic.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


SpatialStatistic::SpatialStatistic() {
	enabled = 0;

	pStatFile = NULL;

}

SpatialStatistic::~SpatialStatistic() {
	
	if(pStatFile != NULL) {
		fclose(pStatFile);
		pStatFile = NULL;
	}

}

int SpatialStatistic::getStats(double tNow, TargetData data, const Raster<int> *simulationState) {

	if (enabled) {
		auto stats = calculateStats(tNow, data, simulationState);

		writeStatsToFile(stats);
	}
	return enabled;
}

void SpatialStatistic::writeStatsToFile(std::map<std::string, double> stats) {

	auto header = statsHeader();
	for (auto headerElement : header) {
		fprintf(pStatFile, "%.9f ", stats[headerElement]);
	}

	fprintf(pStatFile, "\n");

	return;
}

