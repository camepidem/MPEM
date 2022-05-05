//SpatialStatisticRing.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "SpatialStatisticRing.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


SpatialStatisticRing::SpatialStatisticRing()
{

	//Memory housekeeping:
	pStatFile			= NULL;

}


SpatialStatisticRing::~SpatialStatisticRing()
{

	if(pStatFile) {
		fclose(pStatFile);
		pStatFile = NULL;
	}

}

int SpatialStatisticRing::reset() {

	if(enabled) {
		setupFile();
	}

	return enabled;
}

int SpatialStatisticRing::setup(std::vector<TargetData*> data) {

	enabled = 0;

	//Read data from master:
	char szKey[N_MAXSTRLEN];
	char *szStatName = "SpatialStatisticRing";

	sprintf(szKey,"%s%s",szStatName, "Enable");
	readValueToProperty(szKey, &enabled, -2, "[0,1]");


	if(enabled) {

		statConfig.centreX = 0;
		sprintf(szKey,"%s%s",szStatName, "CentreX");
		readValueToProperty(szKey, &statConfig.centreX, 3, "[0,#nCols#)");
		
		statConfig.centreY = 0;
		sprintf(szKey,"%s%s",szStatName, "CentreY");
		readValueToProperty(szKey, &statConfig.centreY, 3, "[0,#nRows#)");

		statConfig.radiusStep = 1.0;
		sprintf(szKey,"%s%s",szStatName, "RadiusStep");
		readValueToProperty(szKey, &statConfig.radiusStep, 3, ">0");

		statConfig.radiusMax = 1.0;
		sprintf(szKey,"%s%s",szStatName, "RadiusMax");
		readValueToProperty(szKey, &statConfig.radiusMax, 3, ">0");

		bOutputFullStats = 0;
		sprintf(szKey,"%s%s",szStatName, "FullStats");
		readValueToProperty(szKey, &bOutputFullStats, -3, "[0,1]");

		dPercentileIncrement = 5.0;
		sprintf(szKey,"%s%s",szStatName, "PercentileIncrement");
		readValueToProperty(szKey, &dPercentileIncrement, -3, ">0");

	}


	if(enabled && !bIsKeyMode()) {

		statConfig.nBins = int(ceil(statConfig.radiusMax / statConfig.radiusStep));

		//Calculate stats on target:
		statConfig.posBins.resize(statConfig.nBins);
		statConfig.negBins.resize(statConfig.nBins);
		statConfig.realAnnulaeSizes.resize(statConfig.nBins);
		for (int iBin = 0; iBin < statConfig.nBins; iBin++) {
			statConfig.realAnnulaeSizes[iBin] = 0;
		}

		for (int iTargetData = 0; iTargetData < data.size(); ++iTargetData) {

			auto targetData = data[iTargetData];

			if (iTargetData == 0) {

				//Allocate data to bins:
				for (int iX = 0; iX < targetData->data.header.nCols; iX++) {
					for (int iY = 0; iY < targetData->data.header.nRows; iY++) {

						double dx = iX - statConfig.centreX;
						double dy = iY - statConfig.centreY;
						double ds = sqrt(dx*dx + dy*dy);

						if (ds < statConfig.radiusMax) {

							int iBin = int(ds / statConfig.radiusStep);

							//Record point in bin:
							statConfig.realAnnulaeSizes[iBin]++;

						}
					}
				}

			}

			//Have we already read this file?
			if (dataStats.find(targetData->id) == dataStats.end()) {
				//Initialise each target data with the config information 
				dataStats[targetData->id] = statConfig;

				//Get comparison stats from target data:
				getRingStats(&targetData->data, &dataStats[targetData->id].posBins[0], &dataStats[targetData->id].negBins[0]);
			}

		}

		setupFile();

	}

	return enabled;
}

std::map<std::string, double> SpatialStatisticRing::calculateStats(double tNow, TargetData data, const Raster<int>* simulationState) {

	std::map<std::string, double> stats;

	stats["Time"] = tNow;

	auto simStats = statConfig;

	getRingStats(simulationState, &simStats.posBins[0], &simStats.negBins[0], &data.data);

	auto dataStat = statConfig;
	if (dataStats.find(data.id) != dataStats.end()) {
		//Already precalculated the stats
		dataStat = dataStats[data.id];
	} else {
		//Need to calculate, as we haven't seen this data before:
		getRingStats(&data.data, &dataStat.posBins[0], &dataStat.negBins[0]);
		//Store it for next time:
		dataStats[data.id] = dataStat;
	}

	//Sum of squares errors:
	int ssErrorPos = 0;
	int ssErrorNeg = 0;
	double ssErrorDensityPos = 0.0;
	double ssErrorDensityNeg = 0.0;
	for (int iBin = 0; iBin<statConfig.nBins; iBin++) {

		int dP = simStats.posBins[iBin] - dataStat.posBins[iBin];
		ssErrorPos += dP*dP;

		int dN = simStats.negBins[iBin] - dataStat.negBins[iBin];
		ssErrorNeg += dN*dN;

		double areaAnnulus = max(1.0, double(statConfig.realAnnulaeSizes[iBin]));//(iBin+1)*(iBin+1) - iBin*iBin;

		double dPd = simStats.posBins[iBin] / areaAnnulus - dataStat.posBins[iBin] / areaAnnulus;
		ssErrorDensityPos += dPd*dPd;

		double dNd = simStats.negBins[iBin] / areaAnnulus - dataStat.negBins[iBin] / areaAnnulus;
		ssErrorDensityNeg += dNd*dNd;

	}

	int ssErrorTotal = ssErrorPos + ssErrorNeg;
	double ssErrorDensityTotal = ssErrorDensityPos + ssErrorDensityNeg;

	stats["SumSquaresErrorPositive"] = ssErrorPos;
	stats["SumSquaresErrorNegative"] = ssErrorNeg;
	stats["SumSquaresErrorTotal"] = ssErrorTotal;
	stats["SumSquaresErrorDensityPositive"] = ssErrorDensityPos;
	stats["SumSquaresErrorDensityNegative"] = ssErrorDensityNeg;
	stats["SumSquaresErrorDensityTotal"] = ssErrorDensityTotal;


	//Chi Squared Esque errors:
	double xsErrorPos = 0.0;
	double xsErrorNeg = 0.0;
	double xsErrorDensityPos = 0.0;
	double xsErrorDensityNeg = 0.0;
	for (int iBin = 0; iBin<statConfig.nBins; iBin++) {

		double simE;

		double dPx = simStats.posBins[iBin] - dataStat.posBins[iBin];
		simE = simStats.posBins[iBin];//Stop it all breaking if there are not meant to be any observed:
		if (simE == 0.0) {
			simE = 0.1;
		}

		xsErrorPos += dPx*dPx / simE;

		double dNx = simStats.negBins[iBin] - dataStat.negBins[iBin];
		simE = simStats.negBins[iBin];//Stop it all breaking if there are not meant to be any observed:
		if (simE == 0.0) {
			simE = 0.1;
		}

		xsErrorNeg += dNx*dNx / simE;

		double areaAnnulus = max(1.0, double(statConfig.realAnnulaeSizes[iBin]));//(iBin+1)*(iBin+1) - iBin*iBin;

		double simEd;

		double dPxd = simStats.posBins[iBin] / areaAnnulus - dataStat.posBins[iBin] / areaAnnulus;
		simEd = simStats.posBins[iBin] / areaAnnulus;//Stop it all breaking if there are not meant to be any observed:
		if (simEd == 0.0) {
			simEd = 0.1;
		}

		xsErrorDensityPos += dPxd*dPxd / simEd;

		double dNxd = simStats.negBins[iBin] / areaAnnulus - dataStat.negBins[iBin] / areaAnnulus;
		simEd = simStats.negBins[iBin] / areaAnnulus;//Stop it all breaking if there are not meant to be any observed:
		if (simEd == 0.0) {
			simEd = 0.1;
		}

		xsErrorDensityNeg += dNxd*dNxd / simEd;

	}

	double xsErrorTotal = xsErrorPos + xsErrorNeg;
	double xsErrorDensityTotal = xsErrorDensityPos + xsErrorDensityNeg;

	stats["ChiSquareErrorPositive"] = xsErrorPos;
	stats["ChiSquareErrorNegative"] = xsErrorNeg;
	stats["ChiSquareErrorTotal"] = xsErrorTotal;
	stats["ChiSquareErrorDensityPositive"] = xsErrorDensityPos;
	stats["ChiSquareErrorDensityNegative"] = xsErrorDensityNeg;
	stats["ChiSquareErrorDensityTotal"] = xsErrorDensityTotal;


	//Percentiles:
	int nTotal = 0;
	for (int iBin = 0; iBin < statConfig.nBins; iBin++) {
		nTotal += simStats.posBins[iBin];
	}

	int iBin = 0;
	int nSum = simStats.posBins[iBin];

	double dPerc = 0.0;
	char statString[_N_MAX_STATIC_BUFF_LEN];
	while (dPerc < 100.0) {

		dPerc += dPercentileIncrement;
		if (dPerc > 100.0) {
			dPerc = 100.0;
		}
		sprintf(statString, "%.1f%%", dPerc);

		while (100.0 * double(nSum) < dPerc * double(nTotal) && iBin < statConfig.nBins) {
			++iBin;//Looks like we could get iBin == nBins, but actually the first condition must fail if iBin == nBins - 1 (floating point horrors notwithstanding...)
			nSum += simStats.posBins[iBin];
		}

		stats[statString] = iBin;
	}

	if (bOutputFullStats) {
		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "D_P_%d", iBin);
			stats[statString] = dataStat.posBins[iBin];
		}

		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "D_N_%d", iBin);
			stats[statString] = dataStat.negBins[iBin];
		}

		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "S_P_%d", iBin);
			stats[statString] = simStats.posBins[iBin];
		}

		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "S_N_%d", iBin);
			stats[statString] = simStats.negBins[iBin];
		}
	}


	return stats;
}

std::vector<std::string> SpatialStatisticRing::statsHeader() {

	std::vector<std::string> header;

	header.push_back("Time");

	//SumSquares
	header.push_back("SumSquaresErrorPositive");
	header.push_back("SumSquaresErrorNegative");
	header.push_back("SumSquaresErrorTotal");
	header.push_back("SumSquaresErrorDensityPositive");
	header.push_back("SumSquaresErrorDensityNegative");
	header.push_back("SumSquaresErrorDensityTotal");

	//ChiSquareEsque
	header.push_back("ChiSquareErrorPositive");
	header.push_back("ChiSquareErrorNegative");
	header.push_back("ChiSquareErrorTotal");
	header.push_back("ChiSquareErrorDensityPositive");
	header.push_back("ChiSquareErrorDensityNegative");
	header.push_back("ChiSquareErrorDensityTotal");

	//Percentiles:
	double dPerc = dPercentileIncrement;
	char statString[_N_MAX_STATIC_BUFF_LEN];
	while (dPerc < 100.0) {
		sprintf(statString, "%.1f%%", dPerc);
		header.push_back(statString);
		dPerc += dPercentileIncrement;
	}
	sprintf(statString, "100.0%%");
	header.push_back(statString);

	if (bOutputFullStats) {
		for (int iBin = 0; iBin < statConfig.nBins; iBin++) {
			sprintf(statString, "D_P_%d", iBin);
			header.push_back(statString);
		}
		for (int iBin = 0; iBin < statConfig.nBins; iBin++) {
			sprintf(statString, "D_N_%d", iBin);
			header.push_back(statString);
		}
		for (int iBin = 0; iBin < statConfig.nBins; iBin++) {
			sprintf(statString, "S_P_%d", iBin);
			header.push_back(statString);
		}
		for (int iBin = 0; iBin < statConfig.nBins; iBin++) {
			sprintf(statString, "S_N_%d", iBin);
			header.push_back(statString);
		}
	}

	return header;
}

void SpatialStatisticRing::setupFile() {

	if(enabled) {
		if(pStatFile) {
			fclose(pStatFile);
		}

		char szFileName[N_MAXFNAMELEN];
		sprintf(szFileName, "%sSpatialStatistics_Ring.txt",szPrefixOutput());
		pStatFile = fopen(szFileName,"w");
		if(pStatFile) {
			;
		} else {
			reportReadError("ERROR: Unable to write to file %s\n", szFileName);
			return;
		}

		//Write header line:
		auto header = statsHeader();
		for (auto headerElement : header) {
			fprintf(pStatFile, "%s ", headerElement.c_str());
		}
		fprintf(pStatFile, "\n");
	}
	return;
}

void SpatialStatisticRing::getRingStats(const Raster<int> *data, int *posBins, int *negBins, const Raster<int> *mask) {

	if(enabled) {
		//Clean data:
		for(int iBin=0; iBin<statConfig.nBins; iBin++) {
			posBins[iBin] = 0;
			negBins[iBin] = 0;
		}

		//Bin current data:
		for(int iX=0; iX<data->header.nCols; iX++) {
			for(int iY=0; iY<data->header.nRows; iY++) {

				int targetValue = data->getValue(iX, iY);
				int maskValue = 1;
				if(mask != NULL) {
					if(mask->getValue(iX, iY) < 0) {
						maskValue = 0;
					}
				}

				if(maskValue && targetValue >= 0) {
					double dx = iX- statConfig.centreX;
					double dy = iY- statConfig.centreY;
					double ds = sqrt(dx*dx+dy*dy);

					if(ds < statConfig.radiusMax) {

						int iBin = int(ds/ statConfig.radiusStep);

						if(targetValue == 0) {
							negBins[iBin]++;
						}
						if(targetValue == 1) {
							posBins[iBin]++;
						}

					}
				}

			}
		}
	}
	return;
}
