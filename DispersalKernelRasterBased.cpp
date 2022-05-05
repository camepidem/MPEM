// DispersalKernelRasterBased.cpp: implementation of DispersalKernelRasterBased class methods.

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include <functional>
#include "DispersalKernelRasterBased.h"

#pragma warning(disable : 4996)

// Construction/Destruction:

DispersalKernelRasterBased::DispersalKernelRasterBased() {

}

DispersalKernelRasterBased::~DispersalKernelRasterBased() {

}

int DispersalKernelRasterBased::init(int iKernel) {

	char szKeyName[_N_MAX_STATIC_BUFF_LEN];

	printk("Reading Kernel data:\n");

	setCurrentWorkingSection("DISPERSAL");

	setCurrentWorkingSubection("Kernel Type", CM_MODEL);

	SKernelData.iKernel = iKernel;

	char tempType[64];
	sprintf(tempType, "Exponential");
	sprintf(szKeyName, "Kernel_%d_Type",iKernel);
	if(readValueToProperty(szKeyName,tempType, 1, "{Exponential, PowerLaw, AdjacentPowerLaw, Raster}")) {
		SKernelData.cKernelType = toupper(tempType[0]);

		//Valid kernels:

		//Raster Based:

		//R : Raster

		//E : Exponential
		//P : Power Law
		//A : Adjacent Power Law

		//TODO: Proper structure for holding all kernel types

	} else {
		printk("Kernel type not specified\n");
		return 0;
	}

	//Read all data for specific types of kernels:


	////Rasterised kernel implementations:////
	SKernelData.bSpecifiedWithinCellProportion = 0;

	//Explicit raster Kernel
	if(SKernelData.cKernelType == 'R' || bIsKeyMode()) {
		setCurrentWorkingSubection("Raster Kernel", CM_MODEL);

		SKernelData.dKernelWithinCellProportion = 0.0;
		sprintf(szKeyName, "Kernel_%d_RasterWithinCellProportion",iKernel);
		SKernelData.bSpecifiedWithinCellProportion = readValueToProperty(szKeyName, &SKernelData.dKernelWithinCellProportion,-2, "[0,1]");

	}

	//Prebuilt functional forms
	if(SKernelData.cKernelType == 'E' || SKernelData.cKernelType == 'P' || SKernelData.cKernelType == 'A' || bIsKeyMode()) {
		setCurrentWorkingSubection("Prebuilt functional forms: Exponential, Power law, Adjacent Power Law", CM_MODEL);

		sprintf(szKeyName, "Kernel_%d_WithinCellProportion",iKernel);
		SKernelData.bSpecifiedWithinCellProportion = 1;
		SKernelData.dKernelWithinCellProportion = 0.0;
		readValueToProperty(szKeyName, &SKernelData.dKernelWithinCellProportion,2, "[0,1]");

		sprintf(szKeyName, "Kernel_%d_Parameter",iKernel);
		SKernelData.dKernelParameter = 1.0;
		readValueToProperty(szKeyName,&SKernelData.dKernelParameter,2, ">=0");
		sprintf(szKeyName, "Kernel_%d_Range",iKernel);
		SKernelData.nKernelLongRange = 1;
		readValueToProperty(szKeyName,&SKernelData.nKernelLongRange,2, ">0");

		SKernelData.dKernelScaling = 1.0;
		sprintf(szKeyName, "Kernel_%d_Scaling",iKernel);
		readValueToProperty(szKeyName,&SKernelData.dKernelScaling,-2, ">0");

	}


	//Common options for Raster implementations of kernels:
	setCurrentWorkingSubection("Kernel Options", CM_APPLICATION);

	SKernelData.dKernelLongCutoff = 0.25;

	double tempCutoff = SKernelData.dKernelLongCutoff;
	sprintf(szKeyName, "Kernel_%d_VirtualSporulationTransition",iKernel);
	if (readValueToProperty(szKeyName, &tempCutoff, -2, "[0,1]")) {
		if(tempCutoff < 0.0 || tempCutoff > 1.0) {
			reportReadError("Kernel Virtual Sporulation transition threshold must be between 0.0 and 1.0.\nObtained: %s = %f\n", szKeyName, tempCutoff);
			return 0;
		} else {
			SKernelData.dKernelLongCutoff = tempCutoff;
		}
	}

	SKernelData.nKernelLongCutoffRange = 5;
	SKernelData.bKernelLongCutoffByRange = 0;
	sprintf(szKeyName, "Kernel_%d_VirtualSporulationTransitionByRange", iKernel);
	if (readValueToProperty(szKeyName, &SKernelData.nKernelLongCutoffRange, -2, ">=0")) {
		SKernelData.bKernelLongCutoffByRange = 1;
		if (SKernelData.nKernelLongCutoffRange < 0) {
			reportReadError("Kernel Virtual Sporulation transition range must be >= 0\nObtained: %s = %f\n", szKeyName, tempCutoff);
			return 0;
		}
	}

	//Force all kernel to be long
	SKernelData.bForceAllVirtualSporulation = 0;
	sprintf(szKeyName, "Kernel_%d_ForceAllVirtualSporulation",iKernel);
	readValueToProperty(szKeyName,&SKernelData.bForceAllVirtualSporulation,-2, "[0,1]");


	SKernelData.bCopy = 0;
	sprintf(szKeyName, "Kernel_%d_Copy",iKernel);
	readValueToProperty(szKeyName,&SKernelData.bCopy,-2, "[0,1]");

	return initFromDataStruct();
}

int DispersalKernelRasterBased::initFromDataStruct(int bMessages) {
	if(!bIsKeyMode()) {
		//Create kernel:

		//Actual reading
		if(SKernelData.cKernelType == 'R') {
			//if the kernel is a raster
			char szKernelFileName[_N_MAX_STATIC_BUFF_LEN];
			sprintf(szKernelFileName, "I_KERNEL_%d.txt",SKernelData.iKernel);
			
			//TODO: Configurable kernel file name

			if (!kernelRaster.init(szKernelFileName, SKernelData.iKernel)) {
				reportReadError("ERROR: DispersalKernel unable to initialise from file %s\n", szKernelFileName);
				return 0;
			}

		} else {
			//Kernel is specified by functional form
			if(bMessages) {
				printf("Generating Dispersal Kernel from specified form\n");
			}

			std::function<double(int, int)> kernelFn;

			switch(SKernelData.cKernelType) {
			case 'E':
				kernelFn = [&](int dx, int dy) {return expKernel(dx, dy); };
				break;
			case 'P':
				kernelFn = [&](int dx, int dy) {return powKernel(dx, dy); };
				break;
			case 'A':
				kernelFn = [&](int dx, int dy) {return adjKernel(dx, dy); };
				break;
			default :
				reportReadError("ERROR: Unknown kernel type!\n");
				reportReadError("Prebuilt functional forms are: Exponential, Power Law, Adjacent Power Law\n\n");
				return 0;
				break;
			}

			if (!SKernelData.bSpecifiedWithinCellProportion) {
				SKernelData.dKernelWithinCellProportion = KernelRasterArray::dWithinCellUnspecified;
			}
			
			if (!kernelRaster.init(SKernelData.nKernelLongRange, kernelFn, SKernelData.nKernelLongCutoffRange, SKernelData.dKernelLongCutoff, SKernelData.bForceAllVirtualSporulation, SKernelData.dKernelWithinCellProportion)) {
				reportReadError("ERROR: DispersalKernel unable to initiailise\n");
				return 0;
			}
			
		}

		if (SKernelData.bCopy) {
			
			char szFileName[_N_MAX_STATIC_BUFF_LEN];
			sprintf(szFileName, "%sI_KERNEL_%d.txt", szPrefixOutput(), SKernelData.iKernel);

			kernelRaster.writeRaster(szFileName);

		}

	}

	return 1;
}

void DispersalKernelRasterBased::reset() {

	//Nothing to do

	return;
}

int DispersalKernelRasterBased::changeParameter(char *szKeyName, double dNewParameter, int bTest) {

	char szTemp[_N_MAX_STATIC_BUFF_LEN];

	sprintf(szTemp, "%s", szKeyName);
	makeUpper(szTemp);

	int bFoundAKey = 0;

	if(strcmp(szKeyName,"PARAMETER") == 0) {
		bFoundAKey = 1;
		if(!bTest) {
			SKernelData.dKernelParameter = dNewParameter;
		}
	}

	if(strcmp(szKeyName,"WITHINCELLPROPORTION") == 0) {
		bFoundAKey = 1;
		if(!bTest) {
			if (dNewParameter < 0.0 || dNewParameter > 1.0) {
				reportReadError("ERROR: When changing kernel parameter, WITHINCELLPROPORTION specified (%.6f) was out of range [0.0, 1.0]\n", dNewParameter);
				return 0;
			}
			SKernelData.bSpecifiedWithinCellProportion = 1;
			SKernelData.dKernelWithinCellProportion = dNewParameter;
		}
	}

	if(bFoundAKey) {
		
		if(!bTest) {
			initFromDataStruct();
		}

		return 1;
	}

	return 0;
}

double DispersalKernelRasterBased::KernelShort(int nDx, int nDy) {
	return kernelRaster.kernelShort(nDx, nDy);
}

int DispersalKernelRasterBased::getShortRange() {
	return kernelRaster.getShortRange();
}

void DispersalKernelRasterBased::KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {
	kernelRaster.kernelVirtualSporulation(tNow, sourceX, sourceY, dDx, dDy);
}

double DispersalKernelRasterBased::getVirtualSporulationProportion() {
	return kernelRaster.getVirtualSporulationProportion();
}

std::vector<double> DispersalKernelRasterBased::getCumulativeSumByRange() {
	return kernelRaster.getCumulativeSumByRange();
}

void DispersalKernelRasterBased::writeSummaryData(char *szSummaryFile) {

	FILE *pSummaryFile = fopen(szSummaryFile, "a");
	if (pSummaryFile) {
		fprintf(pSummaryFile, "%-24s", "KernelType");
		switch (SKernelData.cKernelType) {
			case 'R':
				fprintf(pSummaryFile, "Raster");
				break;
			case 'E':
				fprintf(pSummaryFile, "Exponential");
				break;
			case 'P':
				fprintf(pSummaryFile, "Power Law");
				break;
			case 'A':
				fprintf(pSummaryFile, "Adjacent Power Law");
				break;
		}
		fprintf(pSummaryFile, "\n\n");
		
		fclose(pSummaryFile);

		kernelRaster.writeSummaryData(szSummaryFile);

		FILE *pSummaryFile = fopen(szSummaryFile, "a");
		if (pSummaryFile) {

			if (SKernelData.cKernelType == 'E' || SKernelData.cKernelType == 'P' || SKernelData.cKernelType == 'A') {
				fprintf(pSummaryFile, "%-35s%10.6f\n", "KernelParameter", SKernelData.dKernelParameter);
				fprintf(pSummaryFile, "%-35s%10.6f\n", "KernelScaling", SKernelData.dKernelScaling);
			}

			fclose(pSummaryFile);
		}

	} else {
		printf("ERROR: Dispersal kernel unable to write to summary file.\n");
	}

	return;
}

double DispersalKernelRasterBased::expKernel(int x, int y) {
	double dist2 = x*x+y*y;
	return exp( -SKernelData.dKernelParameter*sqrt(dist2)/SKernelData.dKernelScaling );
}

double DispersalKernelRasterBased::powKernel(int x, int y) {
	double dist2 = x*x+y*y;
	if(dist2 > 0) {
		return pow(sqrt(dist2)/SKernelData.dKernelScaling,-SKernelData.dKernelParameter);
	} else {
		return 1.0;
	}
}

double DispersalKernelRasterBased::adjKernel(int x, int y) {

	double dist2 = x*x+y*y;

	double dCore = (SKernelData.dKernelParameter + 1.0)*0.25;

	double dBody = dCore/(dCore + dist2);

	double dPeak = 0.5;

	double dK = dPeak*pow(dBody, SKernelData.dKernelParameter*0.5);

	return dK;

}
