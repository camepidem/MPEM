//ParameterDistributionUniform.h

#pragma once

#include "ParameterDistribution.h"

class ParameterDistributionUniform : public ParameterDistribution {

public:

	ParameterDistributionUniform();
	virtual ~ParameterDistributionUniform();

	int init(int iDist, Landscape *pWorld);

	int sample();

protected:

	int bUseLogSampling;


	int bControlsRateConstant;
	int iRateConstantsControlledPop, iRateConstantsControlledEvent;

	int bControlsKernel;
	int iKernelToControl;

	double dParameterMin, dParameterMax;

	int logParameters(double dValue);
	
};
