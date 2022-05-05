//SpatialStatisticMoranI.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "SpatialStatisticMoranI.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


SpatialStatisticMoranI::SpatialStatisticMoranI()
{

	//Memory housekeeping:
	pStatFile				= NULL;

}


SpatialStatisticMoranI::~SpatialStatisticMoranI()
{

	if(pStatFile) {
		fclose(pStatFile);
		pStatFile = NULL;
	}

}

int SpatialStatisticMoranI::reset() {

	if(enabled) {
		setupFile();
	}

	return enabled;
}

int SpatialStatisticMoranI::setup(std::vector<TargetData*> data) {

	enabled = 0;

	//Read data from master:
	char szKey[N_MAXSTRLEN];
	char *szStatName = "SpatialStatisticMoranI";

	sprintf(szKey,"%s%s",szStatName, "Enable");
	readValueToProperty(szKey, &enabled, -2, "[0,1]");


	if(enabled) {

		statConfig.radiusStep = 1.0;
		sprintf(szKey,"%s%s",szStatName, "RadiusStep");
		readValueToProperty(szKey, &statConfig.radiusStep, 3, ">0");

		statConfig.radiusMax = 1.0;
		sprintf(szKey,"%s%s",szStatName, "RadiusMax");
		readValueToProperty(szKey, &statConfig.radiusMax, 3, ">0");

		bOutputFullStats = 0;
		sprintf(szKey,"%s%s",szStatName, "FullStats");
		readValueToProperty(szKey, &bOutputFullStats, -3, "[0,1]");

	}


	if(enabled && !bIsKeyMode()) {

		statConfig.nBins = int(ceil(statConfig.radiusMax / statConfig.radiusStep));

		statConfig.moranIToRadius.resize(statConfig.nBins);

		setupFile();
		
		//Calculate size of discs:
		int boundingRadius = int(ceil(statConfig.radiusMax));

		for (int iTargetData = 0; iTargetData < data.size(); ++iTargetData) {

			auto targetData = data[iTargetData];

			//Initialise each target data with the config information 
			dataStats[targetData->id] = statConfig;

			//Calculate stats on target:

			//Get comparison stats from target data:
			getMoranIStats(&targetData->data, &(dataStats[targetData->id].moranIToRadius[0]));

		}

	}

	return enabled;
}

std::map<std::string, double> SpatialStatisticMoranI::calculateStats(double tNow, TargetData data, const Raster<int>* simulationState) {

	std::map<std::string, double> stats;

	stats["Time"] = tNow;

	auto simStats = statConfig;

	getMoranIStats(simulationState, &simStats.moranIToRadius[0], &data.data);

	auto dataStat = statConfig;
	if (dataStats.find(data.id) != dataStats.end()) {
		//Already precalculated the stats
		dataStat = dataStats[data.id];
	} else {
		//Need to calculate, as we haven't seen this data before:
		getMoranIStats(&data.data, &dataStat.moranIToRadius[0]);
		//Store it for next time:
		dataStats[data.id] = dataStat;
	}

	//Sum of squares errors:
	double ssErrorTotal = 0.0;
	for (int iBin = 0; iBin<statConfig.nBins; iBin++) {

		double dP = simStats.moranIToRadius[iBin] - dataStat.moranIToRadius[iBin];
		ssErrorTotal += dP*dP;

	}

	stats["SumSquaresErrorTotal"] = ssErrorTotal;

	//DEBUG:
	if (bOutputFullStats) {
		char statString[_N_MAX_STATIC_BUFF_LEN];

		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "D_R_%d", int((iBin + 1)*statConfig.radiusStep));
			stats[statString] = dataStat.moranIToRadius[iBin];
		}

		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "S_R_%d", int((iBin + 1)*statConfig.radiusStep));
			stats[statString] = simStats.moranIToRadius[iBin];
		}
	}
	//:DEBUG

	return stats;
}

std::vector<std::string> SpatialStatisticMoranI::statsHeader() {

	std::vector<std::string> header;

	header.push_back("Time");

	//SumSquares
	header.push_back("SumSquaresErrorTotal");

	char statString[_N_MAX_STATIC_BUFF_LEN];
	if (bOutputFullStats) {
		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "D_R_%d", int((iBin + 1)*statConfig.radiusStep));
			header.push_back(statString);
		}
		for (int iBin = 0; iBin<statConfig.nBins; iBin++) {
			sprintf(statString, "S_R_%d", int((iBin + 1)*statConfig.radiusStep));
			header.push_back(statString);
		}
	}

	return header;
}

void SpatialStatisticMoranI::setupFile() {

	if(enabled) {
		if(pStatFile) {
			fclose(pStatFile);
		}

		char szFileName[N_MAXFNAMELEN];
		sprintf(szFileName, "%sSpatialStatistics_MoranI.txt",szPrefixOutput());
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

//TODO: Handle the case where there is no infection at all
void SpatialStatisticMoranI::getMoranIStats(const Raster<int> *data, double *MoranIToRadius, const Raster<int> *mask) {

	/*

	For a distribution of N points X_i, i=1...N
	given a distance weighting w_ij between points X_i and X_j

	MoransI = F * T/B

	where

	X = sum i X_i/(N points)

	F = (N points)/(norm of weights)

	T = sum over all pairs i,j : w_ij*(X_i - X)(X_j - X)

	B = sum over all i : (X_i - X)^2

	Here w_ij is 1 if X_i lies within a distance r of X_j and 0 otherwise

	*/

	//First Moment:
	int nPoints = 0;
	double meanVal = 0.0;

	//For each point in landscape:
	for(int centreX=0; centreX<data->header.nCols; centreX++) {
		for(int centreY=0; centreY<data->header.nRows; centreY++) {

			int centreValue = data->getValue(centreX, centreY);
			int maskValue = 1;
			if(mask != NULL) {
				if(mask->getValue(centreX, centreY) < 0) {
					maskValue = 0;
				}
			}

			//That is observed:
			if(maskValue && centreValue >= 0) {

				nPoints++;
				meanVal += centreValue;

			}

		}
	}

	//Average:
	meanVal /= nPoints;

	//Second Moment
	double B = 0.0;

	//For each point in landscape:
	for(int centreX=0; centreX<data->header.nCols; centreX++) {
		for(int centreY=0; centreY<data->header.nRows; centreY++) {

			int centreValue = data->getValue(centreX, centreY);
			int maskValue = 1;
			if(mask != NULL) {
				if(mask->getValue(centreX, centreY) < 0) {
					maskValue = 0;
				}
			}

			//That is observed:
			if(maskValue && centreValue >= 0) {

				B += (centreValue - meanVal)*(centreValue - meanVal);

			}

		}
	}

	//Every single value is the same.
	if (B == 0.0) {
		//We need to divide by B later on, but there is no point doing a lot of work just to print out NaN
		for (int iBin = 0; iBin < statConfig.nBins; iBin++) {
			MoranIToRadius[iBin] = 1.0;
		}
		return;
	}

	//Cross terms:
	vector<double> sigmaW(statConfig.nBins);

	vector<double> T(statConfig.nBins);

	for(int iBin = 0; iBin < statConfig.nBins; iBin++) {

		sigmaW[iBin] = 0.0;

		T[iBin] = 0.0;

	}

	//For each point in landscape:
	for(int centreX = 0; centreX < data->header.nCols; centreX++) {
		//printf("col %d\n",centreX);
		for(int centreY = 0; centreY < data->header.nRows; centreY++) {

			int centreValue = data->getValue(centreX, centreY);
			int maskValue = 1;
			if(mask != NULL) {
				if(mask->getValue(centreX, centreY) < 0) {
					maskValue = 0;
				}
			}

			//That is observed:
			if(maskValue && centreValue >= 0) {

				//For every point within radiusMax
				//Minimal bounding square within landscape:
				int boundingRadius = int(ceil(statConfig.radiusMax));
				int lX=max(0, centreX-boundingRadius);
				int rX=min(data->header.nCols-1, centreX+boundingRadius);
				int lY=max(0, centreY-boundingRadius);
				int rY=min(data->header.nRows-1, centreY+boundingRadius);

				for(int iX=lX; iX<=rX; iX++) {
					for(int iY=lY; iY<=rY; iY++) {

						int targetValue = data->getValue(iX, iY);
						if(targetValue >= 0) {

							double dx = iX-centreX;
							double dy = iY-centreY;
							double ds2 = dx*dx+dy*dy;//sqrt(dx*dx+dy*dy);//TODO: See if removing sqrt and comparing ds^2 to (iBin*radiusStep)^2 if much faster...

							if(ds2 <= statConfig.radiusMax*statConfig.radiusMax && ds2 > 0.0) {//No self interaction

								int minBin = int(ceil(sqrt(ds2) / statConfig.radiusStep)) - 1;

								//Store value into all bins up to largest possible bin:
								for(int iBin=minBin; iBin<statConfig.nBins; iBin++) {

									T[iBin] += (centreValue - meanVal)*(targetValue - meanVal);
									sigmaW[iBin] += 1.0;

								}

							}

						}

					}
				}

			}

		}
	}


	for(int iBin=0; iBin<statConfig.nBins; iBin++) {
		if (sigmaW[iBin] == 0.0) {
			MoranIToRadius[iBin] = 1.0;
		} else {
			double Fbin = nPoints / sigmaW[iBin];

			MoranIToRadius[iBin] = Fbin * T[iBin] / B;
		}
	}

	return;
}
