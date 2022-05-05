// DispersalKernelQuarantineRobin.cpp: implementation of DispersalKernelQuarantineRobin class methods.

#include "stdafx.h"
#include "myRand.h"
#include "cfgutils.h"
#include "DispersalKernelQuarantineRobin.h"

#pragma warning(disable : 4996)

double dKernelParameterShort;
double dKernelParameterLong;

double dShortRangeFraction;

int bUseCauchyForShort;

double dQuarantineEffect;

int nQuarantineStart;
int nQuarantineEnd;

double dAssumedCellsizeInMetres;

double dArbitraryShortCutoffHack;
double dArbitraryLongCutoffHack;

double dFurtherThanTheMoonApogee;

// Construction/Destruction:
DispersalKernelQuarantineRobin::DispersalKernelQuarantineRobin() {

	dAssumedCellsizeInMetres = 250.0;
	readValueToProperty("RobinKernelAssumedCellsizeInMetres", &dAssumedCellsizeInMetres, -1);

	dKernelParameterShort = 20.570;
	readValueToProperty("RobinKernelShortParameter", &dKernelParameterShort, -1);

	dKernelParameterLong = 9500.0;
	readValueToProperty("RobinKernelLongParameter", &dKernelParameterLong, -1);
	
	bUseCauchyForShort = 1;
	readValueToProperty("RobinKernelUseCauchyForShort", &bUseCauchyForShort, -1);

	dShortRangeFraction = 0.9947;
	readValueToProperty("RobinKernelShortRangeFraction", &dShortRangeFraction, -1);

	dQuarantineEffect = 1.0;
	readValueToProperty("RobinKernelQuarantineLongDispersalReduction", &dQuarantineEffect, -1);

	nQuarantineStart = 0;
	readValueToProperty("RobinKernelQuarantineStartX", &nQuarantineStart, -1);

	nQuarantineEnd = 200;
	readValueToProperty("RobinKernelQuarantineStopX", &nQuarantineEnd, -1);

	dFurtherThanTheMoonApogee = 406700001.0;

	dArbitraryShortCutoffHack = dFurtherThanTheMoonApogee;
	readValueToProperty("RobinKernelArbitraryShortCutoffHackDistance", &dArbitraryShortCutoffHack, -1);

	dArbitraryLongCutoffHack = 0.0;
	readValueToProperty("RobinKernelArbitraryLongCutoffHackDistance", &dArbitraryLongCutoffHack, -1);

	dKernelParameterShort /= dAssumedCellsizeInMetres;
	dKernelParameterLong /= dAssumedCellsizeInMetres;

	dArbitraryShortCutoffHack /= dAssumedCellsizeInMetres;
	dArbitraryLongCutoffHack /= dAssumedCellsizeInMetres;

}

DispersalKernelQuarantineRobin::~DispersalKernelQuarantineRobin() {

}

int DispersalKernelQuarantineRobin::init(int iKernel) {



	return 1;
}

void DispersalKernelQuarantineRobin::reset() {

	//Nothing to do

	return;
}

int DispersalKernelQuarantineRobin::changeParameter(char *szKeyName, double dNewParameter, int bTest) {

	dKernelParameterShort = dNewParameter;

	return 1;
}

double DispersalKernelQuarantineRobin::KernelShort(int nDx, int nDy) {

	if (nDx == 0 && nDy == 0) {
		return 0.0;
	}
	return 0.0;
}

int DispersalKernelQuarantineRobin::getShortRange() {
	return 0;
}

void DispersalKernelQuarantineRobin::KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {

	double dKernelParameter;

	if (dUniformRandom() < dShortRangeFraction) {

		dKernelParameter = dKernelParameterShort;

	} else {

		dKernelParameter = dKernelParameterLong;

		if (sourceX >= nQuarantineStart && sourceX <= nQuarantineEnd) {

			if (dUniformRandom() < dQuarantineEffect) {

				dDx = -sourceX - 1;
				dDy = -sourceY - 1;

				return;

			}

		}

	}
	
	double dDist = dKernelParameter*tan(dUniformRandom()*M_PI_2);

	if (dKernelParameter == dKernelParameterShort) {

		if (!bUseCauchyForShort) {
			//Actually use an exponential instead:
			dDist = dExponentialVariate(dKernelParameter);

		}

		if (dDist > dArbitraryShortCutoffHack) {
			dDist = dFurtherThanTheMoonApogee;
		}

	} else {
		if (dDist < dArbitraryLongCutoffHack) {
			dDist = dFurtherThanTheMoonApogee;
		}
	}

	double dTheta = dUniformRandom()*M_PI*2.0;

	dDy = dDist*sin(dTheta);
	dDx = dDist*cos(dTheta);

	return;
}

double DispersalKernelQuarantineRobin::getVirtualSporulationProportion() {
	return 1.0;
}

std::vector<double>CALL DispersalKernelQuarantineRobin::getCumulativeSumByRange() {
	std::vector<double> cumSum(1);
	cumSum[0] = 1.0;

	return cumSum;
}

void DispersalKernelQuarantineRobin::writeSummaryData(char *szSummaryFile) {

	FILE *pSummaryFile = fopen(szSummaryFile, "a");
	if (pSummaryFile) {
		fprintf(pSummaryFile, "%-24s", "KernelType");
		fprintf(pSummaryFile, "Robin Quarantine Cauchy");
		fprintf(pSummaryFile, "\n\n");

		fclose(pSummaryFile);

	} else {
		printf("ERROR: Dispersal kernel unable to write to summary file.\n");
	}

	return;
}
