//InterDetectionSweep.h

//Contains the data for the Output Data Intervention

#ifndef INTERDETECTIONSWEEP_H
#define INTERDETECTIONSWEEP_H 1

#include "Intervention.h"
#include "Raster.h"

class InterDetectionSweep : public Intervention {

public:

	InterDetectionSweep();

	~InterDetectionSweep();

	int intervene();

	int finalAction();

	int dynamic;

	double radius;

	double prob;
	int bDetectionProbabilityRaster;
	Raster<double> probRaster;

	double minDetectableProportion;

	double cost;

	double totalCost;

	int reset();

	int iSurveyRound;

	string detectionFileName;

	int iPopulationToSurvey;

	int bOuputStatus;

	int trackInfectionLevels;

	int samplingStrategy;

	static const int SAMPLING_SEARCHLIST	= 0;
	static const int SAMPLING_FROMWEIGHTING	= 1;

	int surveyStyle;

	static const int SURVEY_OLD			= 0;
	static const int SURVEY_NEW			= 1;

	int nMaxPopSize;

	double fSampleProportion;

	int nSampleSize;

	void writeSummaryData(FILE *pFile);

protected:

	int setupOutput();

	struct SurveyReport {
		int nLocationsSurveyed;
		int nInfectedLocationsFound;
	};

	SurveyReport performSurvey(bool actualSurvey = true);			//Perform a detection sweep over all locations in the landscape that are currently flagged on the search list

	int writeSurveyReport(const SurveyReport &report, int iSurveyRound, int iSurveyWeighting);

	int clearLandscapeSearchStatus();
	int clearLandscapeKnownStatus();

	int writeDetectionHeaders();

	int writeDetectionData(int afterSurvey=0);

	int nLKnown;

	double nGlobalKnownInfectious;
	double nGlobalKnownDetectable;

	//Weighted sampling:
	int nSurveySize; //Number of points available to choose from in a single survey round

	int nSurveyWeightings; //Number of independent sampling startegies to use
	struct SurveyWeightingScheme {
		int nWeighted;		//Total number of locations that available to survey
		vector<double> weightings;	//Weightings of locations
		vector<int> locationIndex;	//Index of locations
	};
	vector<SurveyWeightingScheme> surveyWeightings; // Use the first survey scheme as the actual survey that control etc sees, 
													//the rest are hypothetical surveys that just record their findings for external comparison

	//Actual sampling subroutines:
	vector<int> sample_BT(const SurveyWeightingScheme &weightingScheme);

	int performWeightedSample(const SurveyWeightingScheme &weightingScheme);

	//Apply search status once locations have been found
	int allocateSearchStatus(const vector<int> &sampleLocationIndices);

	double getDetectionParameterForLocation(LocationMultiHost *loc);

};

#endif
