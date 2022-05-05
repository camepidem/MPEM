//InterWeather.h

//Contains the data for the Climate Intervention

#ifndef INTERWEATHER_H
#define INTERWEATHER_H 1

#include "Intervention.h"
#include "WeatherDatabase.h"
#include "WeatherApplication.h"

class InterWeather : public Intervention {

public:

	InterWeather();
	~InterWeather();

	int intervene();
	int reset();

	int finalAction();
	void writeSummaryData(FILE *pFile);

protected:

	WeatherDatabase weatherDB;

	WeatherApplication weatherApp;

	int bFixedColumnSelection;

};

#endif
