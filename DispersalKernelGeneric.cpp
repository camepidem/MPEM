// DispersalKernelGeneric.cpp: implementation of DispersalKernelGeneric class methods.

#include "stdafx.h"
#include "DispersalKernelGeneric.h"

#pragma warning(disable : 4996)

// Construction/Destruction:

DispersalKernelGeneric::DispersalKernelGeneric() {

}

DispersalKernelGeneric::~DispersalKernelGeneric() {

}

int DispersalKernelGeneric::init(int iKernel) {
	return 0;
}

void DispersalKernelGeneric::reset() {
	return;
}

double DispersalKernelGeneric::KernelShort(int nDx, int nDy) {
	return -1.0;
}

int DispersalKernelGeneric::getShortRange(){
	return -1;
}

void DispersalKernelGeneric::KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {

	dDx = -1000000000.0;
	dDy = -1000000000.0;

	return;
}
double DispersalKernelGeneric::getVirtualSporulationProportion() {
	return -1.0;
}

std::vector<double> DispersalKernelGeneric::getCumulativeSumByRange() {
	std::vector<double> cumSum(1);
	cumSum[0] = 1.0;

	return cumSum;
}

void DispersalKernelGeneric::writeSummaryData(char *szSummaryFile) {

	FILE *pSummaryFile = fopen(szSummaryFile, "a");
	if(pSummaryFile) {
		fprintf(pSummaryFile,"ERROR: Generic Dispersal Kernel should not have been used!\n");
		fclose(pSummaryFile);
	} else {
		printf("ERROR: Unable to write to sumary from Generic Dispersal kernel.\nERROR: Generic dispersal kernel should never be used.\n");
	}
	return;
}
