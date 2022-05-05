//ColourRamp.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "ColourRamp.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

ColourRamp::ColourRamp(char *szSource, double dMinValue, double dMaxValue) {

	//TODO: Additional default colour ramps?

	int bCreatedColourRamp = 0;

	//Black-White
	if(szSource == NULL) {
		bCreatedColourRamp = useBlackWhiteColourRamp();
	} else {

		if(strcmp(szSource, "I") == 0) {
			bCreatedColourRamp = useInfectiousColourRamp();
		}

		if(strcmp(szSource, "H") == 0) {
			bCreatedColourRamp = useHealthyColourRamp();
		}
		
		if(strcmp(szSource, "DD") == 0) {
			bCreatedColourRamp = useDiscreteDistinctColourRamp(int(dMinValue), int(dMaxValue));
		}
		
		if(!bCreatedColourRamp) {
			bCreatedColourRamp = readFileToColourRamp(szSource);
		}

	}

	if(!bCreatedColourRamp) {
		fprintf(stderr, "Warning: Couldn't find specified colour ramp, defaulting to black-white\n");
		useBlackWhiteColourRamp();
	}

}

ColourRamp::~ColourRamp() {

}

Colour ColourRamp::getColour(double dValue) {
	return colourLevels[valueToLevel(dValue)];
}

int ColourRamp::getRed(double dValue) {
	return colourLevels[valueToLevel(dValue)].red;
}

int ColourRamp::getGreen(double dValue) {
	return colourLevels[valueToLevel(dValue)].green;
}

int ColourRamp::getBlue(double dValue) {
	return colourLevels[valueToLevel(dValue)].blue;
}

int ColourRamp::valueToLevel(double dVal) {

	if (dVal <= levels[0]) {
		return 0;
	}

	if (dVal >= levels[levels.size() - 1]) {
		return levels.size() - 1;
	}

	//Interval bisect until level is found:
	int iTargetLevel = 0;

	auto low = std::lower_bound(levels.begin(), levels.end(), dVal);

	iTargetLevel = low - levels.begin();

	return iTargetLevel;

}

int ColourRamp::readFileToColourRamp(char *szFileName) {

	ifstream pFile;
	pFile.open(szFileName);

	if(!pFile) {
		//Didn't find file...
		fprintf(stderr, "ERROR: Didnt find the file %s. Enjoy the inevitable crash later!\n", szFileName);
		return 0;
	} else {

		//Get the file length:
		int nLevels = 0;
		string line;
		while(getline(pFile, line)) {
			nLevels++;
		}

		//Allocate the colour ramp storage:
		levels.resize(nLevels);
		colourLevels.resize(levels.size());
		
		//Return to the start:
		pFile.clear();
		pFile.seekg(0,ios::beg);

		int iLevel = 0;
		int r, g, b;
		while(getline(pFile, line)) {
			//Put the data into the channels:
			stringstream linestream(line);

			linestream >> levels[iLevel];
			linestream >> r >> g >> b;

			colourLevels[iLevel] = Colour(r, g, b);

			iLevel++;
		}

		pFile.close();

		return 1;
	}

}

vector<double> ColourRamp::make256Level_0_1_Ramp() {

	int maxLevel = 256;

	vector<double> newLevels;
	newLevels.resize(maxLevel);

	for (int iLevel = 0; iLevel < maxLevel; ++iLevel) {
		newLevels[iLevel] = (double(iLevel) / double(maxLevel-1));
	}

	return newLevels;

}

int ColourRamp::useBlackWhiteColourRamp() {

	//Use the default black/white colour ramp:
	levels = make256Level_0_1_Ramp();
	colourLevels.resize(levels.size());

	for (size_t iLevel = 0; iLevel < colourLevels.size(); ++iLevel) {
		colourLevels[iLevel] = Colour(iLevel, iLevel, iLevel);
	}

	return 1;

}

int ColourRamp::useHealthyColourRamp() {

	//Starts off green if totally uninfected then becomes increasingly darker:
	levels = make256Level_0_1_Ramp();
	colourLevels.resize(levels.size());

	for (size_t iLevel = 0; iLevel < colourLevels.size(); ++iLevel) {
		colourLevels[iLevel] = Colour(0, iLevel, 0);
	}

	return 1;
}

int ColourRamp::useInfectiousColourRamp() {

	//Becomes increasingly darker red:
	levels = make256Level_0_1_Ramp();
	colourLevels.resize(levels.size());

	for (size_t iLevel = 0; iLevel < colourLevels.size(); ++iLevel) {
		colourLevels[iLevel] = Colour(255, 128 - iLevel / 2, 128 - iLevel / 2);
	}

	return 1;
	
}

int ColourRamp::useHealthyInfectiousColourRamp() {

	//Use the default infectious colour ramp:
	useInfectiousColourRamp();

	//But starts off green if totally uninfected:
	colourLevels[0] = Colour(0, 255, 0);
	
	return 1;

}

int ColourRamp::useDiscreteDistinctColourRamp(int nMinValue, int nMaxValue) {

	const int nMaxLevels = 26;
	const int contrastReds[nMaxLevels]		= {043, 255, 000, 240, 153, 076, 025, 000, 255, 128, 148, 143, 157, 194, 000, 255, 255, 066,  94, 000, 224, 116, 153, 255, 255, 255};
	const int contrastGreens[nMaxLevels]	= {206, 000, 117, 163, 063, 000, 025,  92, 204, 128, 255, 124, 204, 000, 051, 164, 168, 102, 241, 153, 255, 010, 000, 255, 225,  80};
	const int contrastBlues[nMaxLevels]		= {072, 016, 220, 255, 000,  92, 025,  49, 153, 128, 181, 000, 000, 136, 128, 005, 187, 000, 242, 143, 102, 255, 000, 128, 000, 005};

	//Don't specify min/max the wrong way round...
	if (nMaxValue < nMinValue) {
		int a = nMaxValue;
		nMaxValue = nMinValue;
		nMinValue = a;
	}

	//Use the maximum contrast discrete colour ramp:
	int nLevels = 1 + nMaxValue - nMinValue;
	
	levels.resize(nLevels);
	colourLevels.resize(levels.size());

	for (size_t iLevel = 0; iLevel < colourLevels.size(); ++iLevel) {
		levels[iLevel] = nMinValue + iLevel;
		colourLevels[iLevel] = Colour(contrastReds[iLevel%nMaxLevels], contrastGreens[iLevel%nMaxLevels], contrastBlues[iLevel%nMaxLevels]);
	}

	return 1;
}
