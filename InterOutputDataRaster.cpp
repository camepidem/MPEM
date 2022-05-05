//InterOutputDataRaster.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "InterventionManager.h"
#include "PopulationCharacteristics.h"
#include "Population.h"
#include "InterOutputDataRaster.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterOutputDataRaster::InterOutputDataRaster() {

	timeSpent = 0;
	InterName = "OutputDataRaster";

	setCurrentWorkingSubection(InterName, CM_SIM);


	//Read Data:
	enabled = 1;
	readValueToProperty("RasterEnable", &enabled, -1, "[0,1]");

	if (enabled || world->bGiveKeys) {

		frequency = 1.0;
		readValueToProperty("RasterFrequency", &frequency, 2, ">0");

		timeFirst = world->timeStart + frequency;
		readValueToProperty("RasterFirst", &timeFirst, 2, ">0");

		//check frequency reasonable
		if (frequency <= 0.0) {
			printk("RasterFrequency negative or longer than simulation, no Raster map data will be recorded apart from final landscape\n");
			//Make it so intervention does not occur
			frequency = 1e30;
		}

		if (timeFirst < world->timeStart) {
			int nPeriodsBehind = ceil((world->timeStart - timeFirst) / frequency);
			double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

			printk("Warning: Raster specified first time %f, which is prior to simulation start time (%f).\nAdvancing first time to earliest time >= simulation start time by advancing start time %d periods to %f.\n", timeFirst, world->timeStart, nPeriodsBehind, newTimeFirst);

			timeFirst = newTimeFirst;
		}

	}

	if (world->bGiveKeys) {

		//Generic instruction on RasterEnable_*POPINDEX*_*CLASSNAME*
		printToFileAndScreen(world->szKeyFileName, 0, "//When raster output is enabled, all rasters are output by default.\n//To output only a subset, disable all rasters and then enable the specific ones desired.\n//Enable individual rasters as: RasterEnable_POPULATIONINDEX_CLASSNAME\n//Example:\n// RasterEnable_0_SUSCEPTIBLE\n");

		printToKeyFile("  RasterDisable_ALL 0 ~i[0,1]~\n");

		printToKeyFile("  RasterEnable_~s{#PopulationIndexAndFullPopulationClassNames#}~* 0 ~i[0,1]~\n");

		printToKeyFile("  RasterEnable_~i[0,#nPopulations#)~_~s{#EpidemiologicalMetrics#}~* 0 ~i[0,1]~\n");

	} else {

		if (enabled) {

			//Set up Intervention:
			
			timeNext = timeFirst;

			//Structure for file outputs:
			char fName[N_MAXFNAMELEN];

			rasterEnabled.resize(PopulationCharacteristics::nPopulations);
			for (int i = 0; i < PopulationCharacteristics::nPopulations; i++) {
				rasterEnabled[i].resize(Population::pGlobalPopStats[i]->nClasses + PopulationCharacteristics::nEpidemiologicalMetrics);
				for (int j = 0; j < Population::pGlobalPopStats[i]->nClasses + PopulationCharacteristics::nEpidemiologicalMetrics; j++) {
					rasterEnabled[i][j] = true;
				}
			}

			int tempInt = 0;
			if (readValueToProperty("RasterDisable_ALL", &tempInt, -2, "[0,1]")) {
				if (tempInt) {
					for (int i = 0; i < PopulationCharacteristics::nPopulations; i++) {
						for (int j = 0; j < Population::pGlobalPopStats[i]->nClasses + PopulationCharacteristics::nEpidemiologicalMetrics; j++) {
							rasterEnabled[i][j] = false;
						}
					}
				}
			}

			//for each possible output raster:
			for (int i = 0; i < PopulationCharacteristics::nPopulations; i++) {
				for (int j = 0; j < Population::pGlobalPopStats[i]->nClasses; j++) {
					sprintf(fName, "RasterEnable_%d_%s", i, Population::pGlobalPopStats[i]->pszClassNames[j]);
					int tempInt = 0;
					if (readValueToProperty(fName, &tempInt, -2)) {
						if (tempInt) {
							rasterEnabled[i][j] = true;
						}
					}
				}
				for (int j = 0; j < PopulationCharacteristics::nEpidemiologicalMetrics; j++) {
					sprintf(fName, "RasterEnable_%d_%s", i, PopulationCharacteristics::vsEpidemiologicalMetricsNames[j].c_str());
					int tempInt = 0;
					if (readValueToProperty(fName, &tempInt, -2)) {
						if (tempInt) {
							rasterEnabled[i][Population::pGlobalPopStats[i]->nClasses + j] = true;
						}
					}
				}
			}
		}

	}

}

InterOutputDataRaster::~InterOutputDataRaster() {
	
}

int InterOutputDataRaster::reset() {

	if (enabled) {
		timeNext = timeFirst;
	}

	return enabled;
}

int InterOutputDataRaster::outputRasters(double dTime) {

	char fName[N_MAXFNAMELEN];

	//Write rasters
	for (int i = 0; i < PopulationCharacteristics::nPopulations; i++) {

		for (int j = 0; j < Population::pGlobalPopStats[i]->nClasses; j++) {
			if (rasterEnabled[i][j]) {
				sprintf(fName, "%s%s%d_%s_%8f%s", szPrefixOutput(), world->szPrefixLandscapeRaster, i, Population::pGlobalPopStats[i]->pszClassNames[j], dTime, world->szSuffixTextFile);
				writeRasterHostAllocation(fName, i, j);
			}
		}
		for (int j = 0; j<PopulationCharacteristics::nEpidemiologicalMetrics; j++) {
			if (rasterEnabled[i][Population::pGlobalPopStats[i]->nClasses + j]) {
				sprintf(fName, "%s%s%d_%s_%8f%s", szPrefixOutput(), world->szPrefixLandscapeRaster, i, PopulationCharacteristics::vsEpidemiologicalMetricsNames[j].c_str(), dTime, world->szSuffixTextFile);
				writeRasterEpidemiologicalMetric(fName, i, j);
			}
		}

	}

	return 1;
}

int InterOutputDataRaster::saveStateData(double dTime) {

	char fName[N_MAXFNAMELEN];

	//Write rasters
	for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations; iPop++) {
		for (int iClass = 0; iClass < Population::pGlobalPopStats[iPop]->nClasses; iClass++) {
			sprintf(fName, "%s%s%d_%s_%8f%s", szPrefixOutput(), world->szPrefixLandscapeRaster, iPop, Population::pGlobalPopStats[iPop]->pszClassNames[iClass], dTime, world->szSuffixTextFile);
			writeRasterHostAllocation(fName, iPop, iClass);
		}
	}

	return 1;
}

int InterOutputDataRaster::intervene() {

	if (enabled) {
		//Avoid double final output by preventing writing within 1% output fq before end of simulation
		if (world->timeEnd - timeNext > 0.01*frequency) {
			//Output rasters:
			outputRasters(timeNext);
		}

		timeNext += frequency;
	}

	return 1;
}

int InterOutputDataRaster::finalAction() {
	//Output the final raster data
	//printf("InterInterOutputDataRaster::finalAction()\n");
	//printf("Creating Final Landscape Rasters\n");

	//Output rasters:
	if (enabled) {
		outputRasters(world->timeEnd);
	}

	return 1;
}

void InterOutputDataRaster::writeSummaryData(FILE *pFile) {
	return;
}
