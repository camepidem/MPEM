//ParameterDistributionPosterior.h

#pragma once

#include "ParameterDistribution.h"

class ParameterDistributionPosterior : public ParameterDistribution {

public:

	ParameterDistributionPosterior();
	virtual ~ParameterDistributionPosterior();

	int init(int iDist, Landscape *pWorld);

	int sample();

protected:

	char szPosteriorFileName[_N_MAX_STATIC_BUFF_LEN];

	CParameterPosterior *pPosterior;

	int bUseLogSampling;

	int logParameters(std::map<std::string, double> params);
	
	int validateParameterName(const char *szParamName);

	std::map<std::string, double> applyParameterChange(const char *szParamName, double dNewValue);

	int changeParameter(const char *szParamName, double dNewValue);

	bool compareParamNames(const char *szParamName1, const char *szParamName2);

};
