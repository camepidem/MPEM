//KernelRasterPoints.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "KernelRasterPoints.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

KernelRasterPoints::KernelRasterPoints() {
	dTotalRelease = 0.0;
}

KernelRasterPoints::KernelRasterPoints(const char *fromFile) {

	//Assign kernel
	//Create storage space for kernel
	if(!init(fromFile)) {
		reportReadError("ERROR: Could not read points formatted kernel from file %s\n", fromFile);
	}

}

KernelRasterPoints::~KernelRasterPoints() {

}

int KernelRasterPoints::init(const char * fromFile) {
	return readKernelRasterPointsfromfile(fromFile);
}

double KernelRasterPoints::kernelShort(int nDx, int nDy) {
	return 0.0;
}

int KernelRasterPoints::getShortRange() {
	return 0;
}

void KernelRasterPoints::kernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {
	
	//perform interval bisection on the kernel array and then return 
	//coordinates of the landing point of the spore relative to the source location in (&dDx, &dDy)

	//finds smallest i such that kernelA[i]>rate by interval bisection
	int nLow,nMid,nHigh, nIndex;

	double rate = dUniformRandom()*dTotalRelease;
	
	if (rate < pdKernelVirtualSporulationArray[nKernelPoints - 1]) {
		//Is a release that will actually be deposited

		nLow = 0;
		nHigh = nKernelPoints - 1;
		while (nHigh - nLow > 1) {
			nMid = nLow + (nHigh - nLow) / 2;
			if (pdKernelVirtualSporulationArray[nMid] > rate) {
				nHigh = nMid;
			} else {
				nLow = nMid;
			}
		}
		if (pdKernelVirtualSporulationArray[nLow] > rate) {
			nIndex = nLow;
		} else {
			nIndex = nHigh;
		}

		//go from nIndex to relative coordinates:
		dDy = pdKernelVirtualSporulationArrayCoordsY[nIndex];
		dDx = pdKernelVirtualSporulationArrayCoordsX[nIndex];

		//Deposition occurrs somewhere uniformly in target cell:
		dDy += (-0.5 + dUniformRandom()) * dCellsizeRatio;
		dDx += (-0.5 + dUniformRandom()) * dCellsizeRatio;

	} else {
		//Deposition didn't happen, throw off the landscape into negative coordinate land:
		dDx = -sourceX - 1000.0;
		dDy = -sourceY - 1000.0;
	}
}

double KernelRasterPoints::getVirtualSporulationProportion() {
	return 1.0;
}

int	KernelRasterPoints::readKernelRasterPointsfromfile(const char *fromFile) {

	//Read kernel raster from file:
	if(!readIntFromCfg(fromFile,"KernelPoints",&nKernelPoints)) {
		reportReadError("ERROR: Could not find \"KernelPoints\" in file %s\n", fromFile);
		return 0;
	}

	int nHeaderLines = 1;
	dCellsizeRatio = 1.0;
	if (readDoubleFromCfg(fromFile, "KernelCellsizeRatio", &dCellsizeRatio)) {
		nHeaderLines++;
	}

	if (dCellsizeRatio <= 0.0) {
		reportReadError("ERROR: KernelCellsizeRatio in file %s is negative (%f)\n", fromFile, dCellsizeRatio);
	}

	int bReadTotalRelease = 0;
	if (readDoubleFromCfg(fromFile, "KernelTotalRelease", &dTotalRelease)) {
		nHeaderLines++;
		bReadTotalRelease = 1;
		if (dTotalRelease < 0.0) {
			reportReadError("ERROR: Total release specified in file %s is negative (%f)", fromFile, dTotalRelease);
		}
	}

	pdKernelVirtualSporulationArray.resize(nKernelPoints);
	pdKernelVirtualSporulationArrayCoordsX.resize(nKernelPoints);
	pdKernelVirtualSporulationArrayCoordsY.resize(nKernelPoints);


	if(!readTable(fromFile, nKernelPoints, nHeaderLines, &pdKernelVirtualSporulationArrayCoordsX[0], &pdKernelVirtualSporulationArrayCoordsY[0], &pdKernelVirtualSporulationArray[0])) {
		reportReadError("ERROR: Could not read Kernel points data from file %s\n", fromFile);
		return 0;
	}

	for (int iVal = 0; iVal < nKernelPoints; iVal++) {
		if (pdKernelVirtualSporulationArray[iVal] < 0) {
			reportReadError("ERROR: Negative value in row %d of kernel file %s\n", iVal, fromFile);
		}
	}

	//Turn values array into cumulative sum for bisection:
	for (int iVal = 1; iVal < nKernelPoints; iVal++) {
		pdKernelVirtualSporulationArray[iVal] += pdKernelVirtualSporulationArray[iVal - 1];
	}

	if (bReadTotalRelease) {
		double dTotalDeposition = pdKernelVirtualSporulationArray[nKernelPoints - 1];
		if (dTotalDeposition > dTotalRelease) {
			if (dTotalDeposition > 1.01 * dTotalRelease) {
				reportReadError("ERROR: Total deposition reported (%f) in kernel file %s is greater than total release specified (%f)\n", dTotalDeposition, fromFile, dTotalRelease);
			} else {
				//Some wiggle room, as NAME has a terrbile integration scheme
				printk("WARNING: Total deposition reported (%f) in kernel file %s is greater than total release specified(%f). Allowing run to continue as error is less than 1%%\n", dTotalDeposition, fromFile, dTotalRelease);
				dTotalRelease = dTotalDeposition;
			}
		}
	} else {
		dTotalRelease = pdKernelVirtualSporulationArray[nKernelPoints - 1];
	}

	return 1;

}
