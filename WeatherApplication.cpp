//WeatherApplication.cpp

#include "stdafx.h"
#include "WeatherModifiers.h"
#include "WeatherDatabase.h"
#include "myRand.h"
#include "cfgutils.h"
#include "WeatherApplication.h"

WeatherApplication::WeatherApplication() {

}

WeatherApplication::~WeatherApplication() {

}

int WeatherApplication::init(string sWeatherFileName, int bFixColumnSelection, WeatherDatabase &weatherDB) {

	bFixedColSelection = bFixColumnSelection;

	//Read weather file:
	ifstream inFile;
	string sLine;

	inFile.open(sWeatherFileName.c_str());

	if(inFile.is_open()) {

		//Discard header:
		getline(inFile, sLine);

		while(inFile.good()) {

			if(getline(inFile, sLine)) {
				weatherElements.push_back(WeatherApplicationElement(sLine, weatherDB));
			}

		}

		inFile.close();

	}

	//TODO: sort to ensure time ordered?

	if(weatherElements.size() == 0) {
		reportReadError("Unable to read any weather data from %s\n", sWeatherFileName.c_str());
	}

	if(bFixedColSelection) {
		//Check is possible to do fixed col selection:
		size_t baseCols = weatherElements[0].vFileIDs.size();
		for(size_t iElement=0; iElement<weatherElements.size(); iElement++) {
			if(weatherElements[iElement].vFileIDs.size() != baseCols) {
				reportReadError("Unable to use fixed column selection in Weather as have differing numbers of file columns specified in %s\n", sWeatherFileName.c_str());
			}
		}
	}

	reset();

	return 1;

}

void WeatherApplication::reset() {
	
	iNextApplicationElement = 0;

	if(bFixedColSelection) {
		iWeatherCol = generateWeatherCol(iNextApplicationElement);
	}

}

double WeatherApplication::dGetTimeOfNextWeatherEvent() {

	//Find next closest weather event:
	if(iNextApplicationElement < weatherElements.size()) {
		return weatherElements[iNextApplicationElement].dTime;
	} else {
		return 1e30;//Huge time
	}
	
}

void WeatherApplication::applyWeatherEvent(double dCurrentTime, WeatherDatabase &weatherDB) {

	//Apply all weather events that have lastTime < time <= currentTime
	while(iNextApplicationElement < weatherElements.size() && weatherElements[iNextApplicationElement].dTime <= dCurrentTime) {

		if(!bFixedColSelection) {
			iWeatherCol = generateWeatherCol(iNextApplicationElement);
		}

		weatherElements[iNextApplicationElement].apply(weatherDB, iWeatherCol);

		iNextApplicationElement++;
	}

}

size_t WeatherApplication::generateWeatherCol(size_t iElement) {
	return size_t(dUniformRandom()*weatherElements[iElement].vFileIDs.size());
}
