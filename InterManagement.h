#pragma once

#include "Intervention.h"
#include "Raster.h"

class InterManagement : public Intervention {

public:

	InterManagement();
	virtual ~InterManagement();

	int intervene();

	int finalAction();

	void writeSummaryData(FILE *pFile);

	int reset();

	//Surveillance:
	std::map<double, Raster<int>*> surveySchedule;

	double pDetection;

	int bLogSurveyDataTable;

	static Raster<int> performSurvey(const Raster<int> &surveyRaster, double detectionProbability);

protected:

	void doSurvey(double dTimeNow);

	double getNextTime(double dTimeNow);

};

