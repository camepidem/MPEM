//InterWeather.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "WeatherModifiers.h"
#include "WeatherDatabase.h"
#include "WeatherApplication.h"
#include "InterWeather.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

InterWeather::InterWeather() {
	timeSpent = 0;
	InterName = "Weather";

	setCurrentWorkingSubection(InterName, CM_APPLICATION);

	//Read Data:
	enabled = 0;
	readValueToProperty("WeatherEnable",&enabled,-1, "[0,1]");

	if(enabled) {

		char weatherFileName[N_MAXFNAMELEN];

		sprintf(weatherFileName, "P_WeatherSwitchTimes.txt");
		readValueToProperty("WeatherFileName", weatherFileName, -2, "#FileName#");

		//TODO: fixed col selection
		bFixedColumnSelection = 0;
		readValueToProperty("WeatherFixFileColumnSelection", &bFixedColumnSelection, -2, "[0,1]");

		if(!world->bReadingErrorFlag && ! bIsKeyMode()) {

			weatherApp.init(weatherFileName, bFixedColumnSelection, weatherDB);

			weatherDB.init(world);

			timeFirst = weatherApp.dGetTimeOfNextWeatherEvent();
			timeNext = timeFirst;

		}
	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterWeather::~InterWeather() {

}

int InterWeather::reset() {

	if(enabled) {

		weatherApp.reset();

		timeNext = weatherApp.dGetTimeOfNextWeatherEvent();

	}

	return enabled;
}


int InterWeather::intervene() {

	weatherApp.applyWeatherEvent(world->time, weatherDB);

	timeNext = weatherApp.dGetTimeOfNextWeatherEvent();

	//Changing all the constants will have trashed all the infection rates:
	world->bFlagNeedsInfectionRateRecalculation();

	return 1;
}

int InterWeather::finalAction() {
	//printf("InterWeather::finalAction()\n");

	return 1;
}

void InterWeather::writeSummaryData(FILE *pFile) {

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Ok\n\n");
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
