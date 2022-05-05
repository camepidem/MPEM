//KernelRaster.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "KernelRasterArray.h"
#include "KernelRasterPoints.h"
#include "KernelRaster.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

KernelRaster::KernelRaster() {
	//
}

KernelRaster::~KernelRaster() {
	//
}

KernelRaster *KernelRaster::createKernelRasterFromFile(char *fromFile, int iKernel) {
	//Create correct implementation of kernelRaster depending on format of file:

	int testInt;

	if(readIntFromCfg(fromFile,"KernelRange",&testInt)) {
		return new KernelRasterArray(fromFile, iKernel);
	}

	if(readIntFromCfg(fromFile,"KernelPoints",&testInt)) {
		return new KernelRasterPoints(fromFile);
	}

	return NULL;

}

double KernelRaster::kernelShort(int nDx, int nDy) {
	return 0.0;
}

int KernelRaster::getShortRange() {
	return 0;
}

void KernelRaster::kernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {
	//Bung off landscape
	dDx = -sourceX;
	dDy = -sourceY;
}

double KernelRaster::getVirtualSporulationProportion() {
	return 0.0;
}
