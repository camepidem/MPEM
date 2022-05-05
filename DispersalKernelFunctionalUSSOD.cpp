// DispersalKernelFunctionalUSSOD.cpp: implementation of DispersalKernelFunctionalUSSOD class methods.

#include "stdafx.h"
#include "myRand.h"
#include "DispersalKernelFunctionalUSSOD.h"

#pragma warning(disable : 4996)

double dKernelInternal;
double dKernelVirtualSporulationProportion;
double dCauchyParameter;

// Construction/Destruction:

DispersalKernelFunctionalUSSOD::DispersalKernelFunctionalUSSOD() {

	dKernelInternal = 0.9947;
	dKernelVirtualSporulationProportion = 1.0 - dKernelInternal;

	dCauchyParameter = 38.102;//cells = 9503.0m/250.0m cells;

}

DispersalKernelFunctionalUSSOD::~DispersalKernelFunctionalUSSOD() {

}

int DispersalKernelFunctionalUSSOD::init(int iKernel) {



	return 1;
}

void DispersalKernelFunctionalUSSOD::reset() {

	//Nothing to do

	return;
}

int DispersalKernelFunctionalUSSOD::changeParameter(char *szKeyName, double dNewParameter, int bTest) {
	
	dCauchyParameter = dNewParameter;
	
	return 1;
}

double DispersalKernelFunctionalUSSOD::KernelShort(int nDx, int nDy) {

	if(nDx == 0 && nDy == 0) {
		return dKernelInternal;
	}
	return 0.0;
}

int DispersalKernelFunctionalUSSOD::getShortRange() {
	return 0;
}

void DispersalKernelFunctionalUSSOD::KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {

	double dDist = dCauchyParameter*tan(dUniformRandom()*M_PI_2);

	double dTheta = dUniformRandom()*M_PI*2.0;

	dDy = dDist*sin(dTheta);
	dDx = dDist*cos(dTheta);

	return;
}

double DispersalKernelFunctionalUSSOD::getVirtualSporulationProportion() {
	return dKernelVirtualSporulationProportion;
}

std::vector<double> DispersalKernelFunctionalUSSOD::getCumulativeSumByRange() {
	std::vector<double> cumSum(1);
	cumSum[0] = 1.0;

	return cumSum;
}

void DispersalKernelFunctionalUSSOD::writeSummaryData(char *szSummaryFile) {

	FILE *pSummaryFile = fopen(szSummaryFile, "a");
	if(pSummaryFile) {
		fprintf(pSummaryFile,"%-24s","KernelType");
		fprintf(pSummaryFile,"US SOD Cauchy");
		fprintf(pSummaryFile,"\n\n");

		fclose(pSummaryFile);

	} else {
		printf("ERROR: Dispersal kernel unable to write to summary file.\n");
	}

	return;
}
