
#include "stdafx.h"
#include "myRand.h"
#include "json/json.h"
#include "cfgutils.h"
#include "DispersalManager.h"
#include "DispersalKernelBlended.h"



DispersalKernelBlended::DispersalKernelBlended() {

}


DispersalKernelBlended::~DispersalKernelBlended() {

}

int DispersalKernelBlended::init(int iKernel) {

	int bInitSuccess = DispersalKernelBlended::getTargetKernelData(iKernel, indices, weights);

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		int targetIndex = indices[iKernel];
		if (targetIndex >= DispersalManager::pRealKernels.size()) {
			reportReadError("ERROR: Blended kernel index: %d is not a valid kernel index - must be less than or equal to maximum kernel index (%d).\n", targetIndex, DispersalManager::pRealKernels.size() - 1);
			bInitSuccess = 0;
		}
	}

	return bInitSuccess;
}

void DispersalKernelBlended::reset() {
	//This does not own the underlying kernels
	return;
}

int DispersalKernelBlended::changeParameter(char *szKeyName, double dNewParameter, int bTest) {
	//This does not own the underlying kernels
	return 0;
}

double DispersalKernelBlended::KernelShort(int nDx, int nDy) {

	double dBlendedKernel = 0.0;

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		dBlendedKernel += weights[iKernel] * DispersalManager::pRealKernels[indices[iKernel]]->KernelShort(nDx, nDy);
	}

	return dBlendedKernel;
}

int DispersalKernelBlended::getShortRange() {

	int nBlendedShortRange = 0;

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		if (weights[iKernel] > 0.0) {//Don't need to use kernels with zero weighting allocation
			int nKernelShortRange = DispersalManager::pRealKernels[indices[iKernel]]->getShortRange();
			if (nKernelShortRange > nBlendedShortRange) {
				nBlendedShortRange = nKernelShortRange;
			}
		}
	}

	return nBlendedShortRange;
}

void DispersalKernelBlended::KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy) {

	double dKernelSelect = dUniformRandom()*getVirtualSporulationProportion();

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		double kernelVSWeighted = weights[iKernel] * DispersalManager::pRealKernels[indices[iKernel]]->getVirtualSporulationProportion();
		if (kernelVSWeighted >= dKernelSelect) {
			DispersalManager::pRealKernels[indices[iKernel]]->KernelVirtualSporulation(tNow, sourceX, sourceY, dDx, dDy);
			return;
		} else {
			dKernelSelect -= kernelVSWeighted;
		}
	}

	return;
}

double DispersalKernelBlended::getVirtualSporulationProportion() {

	double dBlendedVSProp = 0.0;

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		dBlendedVSProp += weights[iKernel] * DispersalManager::pRealKernels[indices[iKernel]]->getVirtualSporulationProportion();
	}

	return dBlendedVSProp;
}

void DispersalKernelBlended::writeSummaryData(char *szSummaryFile) {

	FILE *pSummaryFile = fopen(szSummaryFile, "a");
	if (pSummaryFile) {
		fprintf(pSummaryFile, "%-24s%s\n", "KernelType:", "Blended");
		fprintf(pSummaryFile, "%-24s%d\n", "Number of kernels:", (int)indices.size());
		fprintf(pSummaryFile, "\n");

		fclose(pSummaryFile);

		for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
			pSummaryFile = fopen(szSummaryFile, "a");
			if (pSummaryFile) {
				fprintf(pSummaryFile, "%-24s%d\n", "Kernel:", indices[iKernel]);
				fprintf(pSummaryFile, "%-24s%.6f\n", "Weight:", weights[iKernel]);

				fclose(pSummaryFile);
			} else {
				printf("ERROR: Dispersal kernel unable to write to summary file.\n");
			}

			//This does not own the underlying kernels, they will summarise themselves individually
		}

	} else {
		printf("ERROR: Dispersal kernel unable to write to summary file.\n");
	}

	return;
}

std::vector<double> DispersalKernelBlended::getCumulativeSumByRange() {

	int nMaxLength = 0;

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		auto dTestKernelCumSum = DispersalManager::pRealKernels[indices[iKernel]]->getCumulativeSumByRange();

		if (dTestKernelCumSum.size() > nMaxLength) {
			nMaxLength = dTestKernelCumSum.size();
		}
	}

	std::vector<double> dBlendedCumSum(nMaxLength);

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		auto dTestKernelCumSum = DispersalManager::pRealKernels[indices[iKernel]]->getCumulativeSumByRange();
		
		for (int iRange = 0; iRange < dBlendedCumSum.size(); ++iRange) {
			//Once we go off the end of this particular kernel, we want to use the last value until the end of the blended kernel
			int iKernelRange = iRange;
			if (iKernelRange > dTestKernelCumSum.size() - 1) {
				iKernelRange = dTestKernelCumSum.size() - 1;
			}
			double dWeightedElement = weights[iKernel] * dTestKernelCumSum[iKernelRange];
			
			dBlendedCumSum[iRange] += dWeightedElement;
		}
	}

	return dBlendedCumSum;
}

int DispersalKernelBlended::getTargetKernelData(int iKernel, vector<int> &indices, vector<double> &weights) {

	indices.resize(0);
	weights.resize(0);

	char szKeyName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szKeyName, "Kernel_%d_Blending", iKernel);

	char szConfigStuff[_N_MAX_DYNAMIC_BUFF_LEN];
	sprintf(szConfigStuff, "{\"0\" : 1.0}");
	readValueToProperty(szKeyName, szConfigStuff, 2, "#JSON{i:f}#");

	Json::Reader reader;
	Json::Value root;
	
	if (!reader.parse(szConfigStuff, root)) {
		reportReadError("ERROR: Unable to parse json string : %s, message: %s\n", szConfigStuff, reader.getFormattedErrorMessages().c_str());
		return 0;
	}

	for (auto itr = root.begin(); itr != root.end(); ++itr) {

		//std::cout << itr.key() << " : " << *itr << std::endl;

		//Dealing with strings in C++ is a nightmare:
		std::stringstream ssKey;
		ssKey << itr.key();
		std::string sKey = ssKey.str();
		std::replace(sKey.begin(), sKey.end(), '\"', ' ');

		std::stringstream ssClean;
		ssClean << sKey;

		int targetIndex;// = itr.key().asInt();

		if (!(ssClean >> targetIndex)) {
			reportReadError("ERROR: Kernel index: %s is not a valid kernel index\n", ssClean.str().c_str());
			return 0;
		}

		//Make sure user cant set up loop where things become self referential and a blended kernel uses itself... 
		//TODO: could still be done with multiple steps, and will cause a hang
		if (targetIndex == iKernel) {
			reportReadError("ERROR: Kernel_%d has index %d specified for blending. Recursive blending not allowed. Don't try and sneak it through in multiple steps either.\n", iKernel, targetIndex);
			return 0;
		}

		if (targetIndex < 0) {
			reportReadError("ERROR: Kernel index: %d is not a valid kernel index - must be greater than or equal to zero.\n", targetIndex);
			return 0;
		}

		//Can't check here if kernel index is valid, because they may not all have been read from config yet, but can perform this check seperately when we are actually init'ing a blended kernel


		//Check index not already used (would be hard - keys should be unique in json)
		for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
			if (indices[iKernel] == targetIndex) {
				reportReadError("ERROR: Kernel index: %d is specified more than once.\n", targetIndex);
				return 0;
			}
		}

		if (!(*itr).isDouble()) {
			std::stringstream ssVal;
			ssVal << *itr;
			reportReadError("ERROR: Kernel weighting: %s is not a valid kernel weighting. Must be interpretable as a double\n", ssVal.str().c_str());
			return 0;
		}

		double targetWeight = (*itr).asDouble();

		if (targetWeight < 0.0) {
			reportReadError("ERROR: Kernel weighting: %f is not a valid kernel weighting - must be greater than or equal to zero.\n", targetWeight);
			return 0;
		}

		indices.push_back(targetIndex);
		weights.push_back(targetWeight);
	}

	double dTotalWeight = 0.0;

	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		dTotalWeight += weights[iKernel];
	}

	if (dTotalWeight <= 0.0) {
		reportReadError("ERROR: Total weighting for blended kernels (%f) must be positive\n", dTotalWeight);
		return 0;
	}
	
	for (int iKernel = 0; iKernel < indices.size(); ++iKernel) {
		weights[iKernel] /= dTotalWeight;
	}

	return 1;
}

