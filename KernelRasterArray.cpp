//KernelRasterArray.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include "KernelRasterArray.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

const double KernelRasterArray::dWithinCellUnspecified = -1.2345;

KernelRasterArray::KernelRasterArray() {
	init(0, [](int dx, int dy) { if (dx == 0 && dy == 0) { return 1.0; } else { return 0.0; } }, 0, 1.0);
}

KernelRasterArray::KernelRasterArray(char *fromFile, int iKernel) {
	init(fromFile, iKernel);
}

KernelRasterArray::KernelRasterArray(char * fromFile, int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation, double dWithinCellProportion) {
	init(fromFile, nShortRange, dVSTransitionThreshold, bForceAllVirtualSporulation, dWithinCellProportion);
}

KernelRasterArray::KernelRasterArray(int nKernelRange, std::function<double(int, int)> kernelFunction, int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation, double dWithinCellProportion) {
	init(nKernelRange, kernelFunction, nShortRange, dVSTransitionThreshold, bForceAllVirtualSporulation, dWithinCellProportion);
}

KernelRasterArray::~KernelRasterArray() {

}

double KernelRasterArray::kernelShort(int nDx, int nDy) {
	if (abs(nDx) <= nKernelShortRange && abs(nDy) <= nKernelShortRange) {
		return pdKernelShortArray[nKernelShortRange + nDx + (1 + 2 * nKernelShortRange)*(nKernelShortRange + nDy)];
	}
	return 0.0;
}

int KernelRasterArray::getShortRange() {
	return nKernelShortRange;
}

void KernelRasterArray::kernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {
	//perform interval bisection on the long kernel array and then return 
	//coordinates of the landing point of the spore relative to the source location in (&dx, &dy)

	//finds smallest i such that kernelLongA[i]>rate by interval bisection
	int nLow,nMid,nHigh, nIndex;

	double rate = dUniformRandom() * dKernelVirtualSporulationProportion;


	nLow = 0;
	nHigh = pdKernelVirtualSporulationArray.size() - 1;
	while(nHigh - nLow > 1) {
		nMid = nLow + (nHigh - nLow)/2;
		if(pdKernelVirtualSporulationArray[nMid] > rate) {
			nHigh = nMid;
		} else {
			nLow = nMid;
		}
	}
	if(pdKernelVirtualSporulationArray[nLow] > rate) {
		nIndex =  nLow;
	} else {
		nIndex = nHigh;
	}

	//go from nIndex to relative coordinates:

	dDy = nIndex / (2 * nKernelLongRange + 1);
	dDx = nIndex - dDy * (2 * nKernelLongRange + 1);

	//Translate so relative to origin of raster:
	dDy -= nKernelLongRange;
	dDx -= nKernelLongRange;

	return;
}

double KernelRasterArray::getVirtualSporulationProportion() {
	return dKernelVirtualSporulationProportion;
}

int KernelRasterArray::init(char * fromFile, int iKernel) {

	int bReadSuccess = 1;

	//Read kernel raster from file:

	char szKeyName[_N_MAX_STATIC_BUFF_LEN];

	printf("Reading Dispersal Kernel Array from file %s\n", fromFile);

	int bSpecifiedWithinCellProportion = 0;
	dKernelWithinCellProportion = dWithinCellUnspecified;
	sprintf(szKeyName, "Kernel_%d_RasterWithinCellProportion", iKernel);
	bSpecifiedWithinCellProportion = readValueToProperty(szKeyName, &dKernelWithinCellProportion, -2, "[0,1]");

	double dKernelLongCutoff = 0.25;

	double tempCutoff = dKernelLongCutoff;
	sprintf(szKeyName, "Kernel_%d_VirtualSporulationTransition", iKernel);
	if (readValueToProperty(szKeyName, &tempCutoff, -2), "[0,1]") {
		if (tempCutoff < 0.0 || tempCutoff > 1.0) {
			reportReadError("Kernel Virtual Sporulation transition threshold must be between 0.0 and 1.0.\nObtained: %s = %f\n", szKeyName, tempCutoff);
		} else {
			dKernelLongCutoff = tempCutoff;
		}
	}

	int nKernelLongCutoffRange = 5;
	int bKernelLongCutoffByRange = 0;
	sprintf(szKeyName, "Kernel_%d_VirtualSporulationTransitionByRange", iKernel);
	if (readValueToProperty(szKeyName, &nKernelLongCutoffRange, -2), ">=0") {
		bKernelLongCutoffByRange = 1;
		if (nKernelLongCutoffRange < 0) {
			reportReadError("Kernel Virtual Sporulation transition range must be >= 0\nObtained: %s = %d\n", szKeyName, nKernelLongCutoffRange);
		}
	}

	//Force all kernel to be long
	int bForceAllVirtualSporulation = 0;
	sprintf(szKeyName, "Kernel_%d_ForceAllVirtualSporulation", iKernel);
	readValueToProperty(szKeyName, &bForceAllVirtualSporulation, -2, "[0,1]");


	int kernelCopy = 0;
	sprintf(szKeyName, "Kernel_%d_Copy", iKernel);
	readValueToProperty(szKeyName, &kernelCopy, -2, "[0,1]");

	if (!bIsKeyMode()) {

		bReadSuccess = init(fromFile, nKernelLongCutoffRange, dKernelLongCutoff, (bool)bForceAllVirtualSporulation, dKernelWithinCellProportion);

		//DEBUG: display raster kernel
		if (kernelCopy) {
			//user wants reference copy of dispersal kernel
			sprintf(szKeyName, "%sI_KERNEL_%d.txt", szPrefixOutput(), iKernel);
			writeRaster(szKeyName);
		}

	}

	return bReadSuccess;
}

int KernelRasterArray::init(char * fromFile, int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation, double dWithinCellProportion) {
	
	int bReadSuccess = 1;

	if (!readIntFromCfg(fromFile, "KernelRange", &nKernelLongRange)) {
		bReadSuccess = 0;
		reportReadError("ERROR: Could not find \"KernelRange\" in file %s\n", fromFile);
	}

	int nKWidth = 1 + 2 * nKernelLongRange;
	int kernelArea = nKWidth * nKWidth;

	pdKernelVirtualSporulationArray.resize(kernelArea);

	if (!readRasterToKernel(fromFile, nKernelLongRange, &pdKernelVirtualSporulationArray[0])) {
		bReadSuccess = 0;
		reportReadError("ERROR: Could not read kernel from file %s\n", fromFile);
	}
	
	return bReadSuccess && compose(nShortRange, dVSTransitionThreshold, bForceAllVirtualSporulation, dWithinCellProportion);

}

int KernelRasterArray::init(int nKernelRange, std::function<double(int, int)> kernelFunction, int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation, double dWithinCellProportion) {

	nKernelLongRange = nKernelRange;

	int nKernelWidth = 1 + 2 * nKernelLongRange;
	int kernelArea = nKernelWidth * nKernelWidth;
	
	pdKernelVirtualSporulationArray.resize(kernelArea);

	for (int dY = -nKernelRange; dY <= nKernelRange; ++dY) {
		for (int dX = -nKernelRange; dX <= nKernelRange; ++dX) {
			pdKernelVirtualSporulationArray[nKernelLongRange + dX + nKernelWidth * (nKernelLongRange + dY)] = kernelFunction(dX, dY);
		}
	}

	compose(nShortRange, dVSTransitionThreshold, bForceAllVirtualSporulation, dWithinCellProportion);

	return 1;
}

int KernelRasterArray::changeComposition(int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation) {

	//TODO: Can do this more efficiently if we only update the bit that changes -might not be a big saving, as even the smallest change possible requires updating half the cumulative sum

	//Un-cumulative sum the VS array
	for (int i = pdKernelVirtualSporulationArray.size() - 1; i > 0; --i) {
		pdKernelVirtualSporulationArray[i] -= pdKernelVirtualSporulationArray[i - 1];
	}

	//Put the short kernel back into the VS array
	int nKShortWidth = 1 + 2 * nKernelShortRange;
	int nKLongWidth = 1 + 2 * nKernelLongRange;

	for (int i = -nKernelShortRange; i <= nKernelShortRange; i++) {
		for (int j = -nKernelShortRange; j <= nKernelShortRange; j++) {

			int iKernelShortCell = i + nKernelShortRange + nKShortWidth*(j + nKernelShortRange);

			int iKernelLongCell = i + nKernelLongRange + nKLongWidth*(j + nKernelLongRange);

			pdKernelVirtualSporulationArray[iKernelLongCell] = pdKernelShortArray[iKernelShortCell];
		}
	}

	//Recompose:
	compose(nShortRange, dVSTransitionThreshold, bForceAllVirtualSporulation);

	return 1;
}

std::vector<double> KernelRasterArray::getCumulativeSumByRange() {

	auto kernelValueArray = pdKernelVirtualSporulationArray;

	//Un-cumulative sum the VS array
	for (int i = kernelValueArray.size() - 1; i > 0; --i) {
		kernelValueArray[i] -= kernelValueArray[i - 1];
	}

	//Put the short kernel back into the VS array
	int nKShortWidth = 1 + 2 * nKernelShortRange;
	int nKLongWidth = 1 + 2 * nKernelLongRange;

	for (int i = -nKernelShortRange; i <= nKernelShortRange; i++) {
		for (int j = -nKernelShortRange; j <= nKernelShortRange; j++) {

			int iKernelShortCell = i + nKernelShortRange + nKShortWidth*(j + nKernelShortRange);

			int iKernelLongCell = i + nKernelLongRange + nKLongWidth*(j + nKernelLongRange);

			kernelValueArray[iKernelLongCell] = pdKernelShortArray[iKernelShortCell];
		}
	}


	std::vector<double> cumSum(nKernelLongRange + 1);
	double dPartialSum = kernelValueArray[nKernelLongRange + nKernelLongRange * nKLongWidth];
	cumSum[0] = dPartialSum;
	for (int r = 1; r <= nKernelLongRange; r++) {

		//top and bottom:
		//y = +/-r, x = [-r,r]
		for (int j = -r; j <= r; j++) {
			int x = nKernelLongRange + j;
			//top:
			int y = nKernelLongRange + r;
			int yC = y*nKLongWidth;
			dPartialSum += kernelValueArray[x + yC];

			//bottom
			y = nKernelLongRange - r;
			yC = y*nKLongWidth;
			dPartialSum += kernelValueArray[x + yC];
		}

		//sides:
		//x = +/-r, y = (-r,r)
		for (int j = -r + 1; j < r; j++) {//not the corners again
			int y = nKernelLongRange + j;
			int yC = y*nKLongWidth;
			//left:
			int x = nKernelLongRange - r;
			dPartialSum += kernelValueArray[x + yC];

			//right
			x = nKernelLongRange + r;
			dPartialSum += kernelValueArray[x + yC];
		}

		cumSum[r] = dPartialSum;

	}

	return cumSum;
}


int KernelRasterArray::compose(int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation, double dWithinCellProportion) {

	int nKLongWidth = 1 + 2 * nKernelLongRange;
	int iKernelCentreCell = nKernelLongRange + nKernelLongRange*nKLongWidth;
	int kernelArea = nKLongWidth * nKLongWidth;

	//Detemine how to split the kernel between short and long range
	double dTotalKernel = 0.0;//Total external kernel interaction

	for (int i = 0; i < kernelArea; i++) {
		double kernelVal = pdKernelVirtualSporulationArray[i];
		if (kernelVal < 0.0) {
			reportReadError("ERROR: Kernel weighting is < 0.0\n");
		}
		dTotalKernel += kernelVal;
	}

	if (dTotalKernel <= 0.0) {
		reportReadError("ERROR: Total kernel probability is <= 0.0\n");
	}

	//If have not specified for raster to override 
	if (dWithinCellProportion == KernelRasterArray::dWithinCellUnspecified) {
		//Make within cell proportion the proportion it already is:
		dKernelWithinCellProportion = pdKernelVirtualSporulationArray[iKernelCentreCell] / dTotalKernel;
	} else {
		dKernelWithinCellProportion = dWithinCellProportion;
	}

	
	double dKernelOutsideTotal = dTotalKernel - pdKernelVirtualSporulationArray[iKernelCentreCell];

	if (dKernelWithinCellProportion < 1.0 && dKernelOutsideTotal == 0.0) {
		dKernelWithinCellProportion = 1.0;
	}

	pdKernelVirtualSporulationArray[iKernelCentreCell] = dKernelWithinCellProportion;

	double dOutsideNormalisationFactor = 0.0;
	if (dKernelOutsideTotal > 0.0) {
		dOutsideNormalisationFactor = (1.0 - dKernelWithinCellProportion) / dKernelOutsideTotal;
	}

	//Normalise Kernel:
	dTotalKernel = dKernelWithinCellProportion;
	for (int i = 0; i < kernelArea; i++) {
		if (i != iKernelCentreCell) {
			pdKernelVirtualSporulationArray[i] *= dOutsideNormalisationFactor;
			dTotalKernel += pdKernelVirtualSporulationArray[i];
		}
	}

	if (abs(dTotalKernel - 1.0) > 1e-3) {
		reportReadError("ERROR: Kernel normalisation error, normalised kernel sums to %f, which is different to 1.0 by more than 1e-3\n", dTotalKernel);
	}

	//TODO: Optimisation: Might want to truncate kernel at the range = size of the landscape?

	
	double dShortSum = 0.0;
	int rMin = 0;

	//If have not selected to use all long range, find cutoff for hybrid kernels:
	if (!bForceAllVirtualSporulation) {

		//Now find (square) radius such that sum contribution of kernel > cutoff
		double dPartialSum = pdKernelVirtualSporulationArray[iKernelCentreCell];
		dShortSum = dPartialSum;

		rMin = min(nShortRange, nKernelLongRange);

		//Unless have already exceeded the necessary proportion of dispersal
		if (dPartialSum <= dVSTransitionThreshold * dTotalKernel) {

			for (int r = 1; r <= rMin; r++) {

				int x, y;
				int yC;

				//top and bottom:
				//y = +/-r, x = [-r,r]
				for (int j = -r; j <= r; j++) {
					x = nKernelLongRange + j;
					//top:
					y = nKernelLongRange + r;
					yC = y*nKLongWidth;
					dPartialSum += pdKernelVirtualSporulationArray[x + yC];

					//bottom
					y = nKernelLongRange - r;
					yC = y*nKLongWidth;
					dPartialSum += pdKernelVirtualSporulationArray[x + yC];
				}

				//sides:
				//x = +/-r, y = (-r,r)
				for (int j = -r + 1; j < r; j++) {//not the corners again
					y = nKernelLongRange + j;
					yC = y*nKLongWidth;
					//left:
					x = nKernelLongRange - r;
					dPartialSum += pdKernelVirtualSporulationArray[x + yC];

					//right
					x = nKernelLongRange + r;
					dPartialSum += pdKernelVirtualSporulationArray[x + yC];
				}

				dShortSum = dPartialSum;

				if (dPartialSum > dVSTransitionThreshold * dTotalKernel) {
					if (r < rMin) {//Consistency with range/proportion specifications
						rMin = r;
					}
					break;
				}

			}

		}

	}

	//Write short range and long range components of the dispersal kernel:

	//Set properties of long and short kernels:

	//Long Kernel:

	//Quantity of kernel still contained within the long range kernel:
	dKernelVirtualSporulationProportion = dTotalKernel - dShortSum;


	//Short Kernel:
	nKernelShortRange = rMin;
	int nKShortWidth = (1 + 2 * nKernelShortRange);
	int kernelShortArea = nKShortWidth * nKShortWidth;


	//Make short and long kernels from the temp Kernel:

	//Short kernel:
	pdKernelShortArray.resize(kernelShortArea);

	//Copy region of temp kernel to be used as short kernel
	//and zero out parts of temp kernel already included in the short kernel

	if (!bForceAllVirtualSporulation) {
		for (int i = -nKernelShortRange; i <= nKernelShortRange; i++) {
			for (int j = -nKernelShortRange; j <= nKernelShortRange; j++) {

				int iKernelShortCell = i + nKernelShortRange + nKShortWidth*(j + nKernelShortRange);

				int iKernelLongCell = i + nKernelLongRange + nKLongWidth*(j + nKernelLongRange);

				pdKernelShortArray[iKernelShortCell] = pdKernelVirtualSporulationArray[iKernelLongCell];
				pdKernelVirtualSporulationArray[iKernelLongCell] = 0.0;
			}
		}
	} else {
		//Just make the centre (only) cell zero so the normal kernel does nothing:
		pdKernelShortArray[0] = 0.0;
	}

	//Long Kernel:
	//Long kernel is actually stored as a cumulative sum
	for (int i = 1; i < pdKernelVirtualSporulationArray.size(); i++) {
		pdKernelVirtualSporulationArray[i] += pdKernelVirtualSporulationArray[i - 1];
	}

	return 1;
}

int KernelRasterArray::writeRaster(const char * fileName) {

	FILE *pKERFILE;
	int maxW = 2 * nKernelLongRange + 1;
	
	pKERFILE = fopen(fileName, "w");
	if (pKERFILE) {
		char *buff = new char[_N_MAX_DYNAMIC_BUFF_LEN];
		char *ptr = buff;
		//write kernel header
		fprintf(pKERFILE, "KernelRange %d\n", nKernelLongRange);
		//write kernel Raster:
		for (int j = 0; j < maxW; j++) {
			for (int i = 0; i < maxW; i++) {
				int index = i + j*maxW;
				double kernelLongCpt = pdKernelVirtualSporulationArray[index];
				if (index > 0) {
					kernelLongCpt -= pdKernelVirtualSporulationArray[index - 1];
				}
				ptr += sprintf(ptr, "%1.10f ", (kernelLongCpt + kernelShort(i - nKernelLongRange, j - nKernelLongRange)));
			}
			sprintf(ptr, "\n");
			fprintf(pKERFILE, "%s", buff);
			ptr = buff;
		}
		fclose(pKERFILE);
		delete[] buff;
	}

	return 1;
}

void KernelRasterArray::writeSummaryData(char * szSummaryFile) {

	FILE *pSummaryFile = fopen(szSummaryFile, "a");
	if (pSummaryFile) {

		fprintf(pSummaryFile, "%-37s%.6f\n", "KernelVirtualSporulationTransition", 1.0 - dKernelVirtualSporulationProportion);
		fprintf(pSummaryFile, "%-32s%6d\n", "KernelShortRange", nKernelShortRange);
		fprintf(pSummaryFile, "%-32s%6d\n", "KernelVirtualSporulationRange", nKernelLongRange);
		fprintf(pSummaryFile, "%-35s%10.6f\n", "KernelWithinCellProportion", dKernelWithinCellProportion);

		fclose(pSummaryFile);

	}

}
