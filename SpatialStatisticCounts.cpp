//SpatialStatisticCounts.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "SpatialStatisticCounts.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


SpatialStatisticCounts::SpatialStatisticCounts()
{

	pStatFile = NULL;

}


SpatialStatisticCounts::~SpatialStatisticCounts()
{

	if(pStatFile != NULL) {
		fclose(pStatFile);
		pStatFile = NULL;
	}

}

int SpatialStatisticCounts::reset() {

	if(enabled) {
		setupFile();
	}

	return enabled;
}

int SpatialStatisticCounts::setup(std::vector<TargetData*> data) {

	enabled = 0;

	char szKey[N_MAXSTRLEN];
	char *szStatName = "SpatialStatisticCounts";

	sprintf(szKey,"%s%s",szStatName, "Enable");
	readValueToProperty(szKey, &enabled, -2, "[0,1]");

	if(enabled && !bIsKeyMode()) {
		setupFile();
	}

	return enabled;
}

std::map<std::string, double> SpatialStatisticCounts::calculateStats(double tNow, TargetData data, const Raster<int>* simulationState) {

	int nSimPos, nSimNeg, nSimTruePos, nSimTrueNeg, nSimFalsePos, nSimFalseNeg;
	getCounts(&data.data, simulationState, nSimPos, nSimNeg, nSimTruePos, nSimTrueNeg, nSimFalsePos, nSimFalseNeg, &data.data);

	int nDataPos = nSimTruePos + nSimFalseNeg;
	int nDataNeg = nSimTrueNeg + nSimFalsePos;

	int nEP = nSimPos - nDataPos;
	nEP *= nEP;
	int nEN = nSimNeg - nDataNeg;
	nEN *= nEN;

	int nError = nEP + nEN;

	double xEP = nEP;
	if (nSimPos > 0) {
		xEP /= nSimPos;
	}
	double xEN = nEN;
	if (nSimNeg > 0) {
		xEN /= nSimNeg;
	}

	double xError = xEP + xEN;


	std::map<std::string, double> stats;

	stats["Time"] = tNow;

	stats["DataCountPositive"] = nDataPos;
	stats["DataCountNegative"] = nDataNeg;

	stats["SimCountPositive"] = nSimPos;
	stats["SimCountNegative"] = nSimNeg;

	stats["CountErrorSumSquares"] = nError;
	stats["CountErrorChiSquare"] = xError;

	stats["SimTruePositive"] = nSimTruePos;
	stats["SimTrueNegative"] = nSimTrueNeg;

	stats["SimFalsePositive"] = nSimFalsePos;
	stats["SimFalseNegative"] = nSimFalseNeg;


	return stats;
}

std::vector<std::string> SpatialStatisticCounts::statsHeader() {

	std::vector<std::string> header;

	header.push_back("Time");

	header.push_back("DataCountPositive");
	header.push_back("DataCountNegative");

	header.push_back("SimCountPositive");
	header.push_back("SimCountNegative");

	header.push_back("CountErrorSumSquares");
	header.push_back("CountErrorChiSquare");

	header.push_back("SimTruePositive");
	header.push_back("SimTrueNegative");

	header.push_back("SimFalsePositive");
	header.push_back("SimFalseNegative");

	return header;
}

void SpatialStatisticCounts::setupFile() {

	if(enabled) {
		if(pStatFile) {
			fclose(pStatFile);
			pStatFile = NULL;
		}

		char szFileName[N_MAXFNAMELEN];
		sprintf(szFileName, "%sSpatialStatistics_Counts.txt",szPrefixOutput());
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

void SpatialStatisticCounts::getCounts(const Raster<int> *data, const Raster<int> *simulationState, int &nPos, int &nNeg, int &nTruePos, int &nTrueNeg, int &nFalsePos, int &nFalseNeg, const Raster<int> *mask) {

	//Clean data:
	nPos = 0;
	nNeg = 0;
	nTruePos = 0;
	nTrueNeg = 0;
	nFalsePos = 0;
	nFalseNeg = 0;

	//Count current data:
	for(int iX=0; iX<data->header.nCols; iX++) {
		for(int iY=0; iY<data->header.nRows; iY++) {

			int simValue = simulationState->getValue(iX, iY);
			int dataValue = data->getValue(iX, iY);
			int maskValue = 1;
			if(mask != NULL) {
				if(mask->getValue(iX, iY) < 0) {
					maskValue = 0;
				}
			}

			if(maskValue && simValue >= 0 && dataValue >= 0) {

				if(simValue == 0) {
					nNeg++;
					if(dataValue == 0) {
						nTrueNeg++;
					} else {
						nFalseNeg++;
					}
				}
				if(simValue == 1) {
					nPos++;
					if(dataValue == 1) {
						nTruePos++;
					} else {
						nFalsePos++;
					}
				}

			}

		}
	}

	return;
}
