//InterOutputDataRaster.h

//Contains the data for the Output Data Intervention

#ifndef INTEROUTPUTDATARASTER_H
#define INTEROUTPUTDATARASTER_H 1

#include "Intervention.h"

class InterOutputDataRaster : public Intervention {

public:

	InterOutputDataRaster();

	~InterOutputDataRaster();

	int suppressAll;
	
	//int suppress();

	int intervene();

	int finalAction();

	int outputRasters(double dTime);

	int saveStateData(double dTime);

	void writeSummaryData(FILE *pFile);

	int reset();

protected:

	vector<vector<bool>> rasterEnabled;

};

#endif
