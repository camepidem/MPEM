//CParameterPosterior.h

#pragma once

class CParameterPosterior {

public:

	CParameterPosterior();
	~CParameterPosterior();

	int init(char *fromFile);
	int write(char *toFile);

	//Returns array of parameters drawn from posterior distribution (having also selected from within parameter intervals)
	double *pdDrawFromPosterior();

	int nParameters;
	vector<string> pszParameterNames;
	
	std::map<std::string, std::map<std::string, double>> parameterCorrelations;

protected:

	int nEntries;
	RateStructure *rsdDensityArray;
	
	vector<int> pbSampleLogSpace;
	vector<vector<double>> ppdParameterIntervalLowers;
	vector<vector<double>> ppdParameterIntervalUppers;
	vector<double> pdParameterDraw;

};
