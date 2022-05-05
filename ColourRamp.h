//ColourRamp.h

#pragma once

class Colour {

public:

	Colour() : red(0), green(0), blue(0) {};
	Colour(int r, int g, int b) : red(r), green(g), blue(b) {};

	int red;
	int green;
	int blue;

};

class ColourRamp {

public:

	ColourRamp(char *szSource = NULL, double dMinValue = 0.0, double dMaxValue = 1.0);
	~ColourRamp();

	Colour getColour(double dValue);

	int getRed(double dValue);
	int getGreen(double dValue);
	int getBlue(double dValue);

protected:

	std::vector<double> levels;
	std::vector<Colour> colourLevels;

	int valueToLevel(double dVal);

	int readFileToColourRamp(char *szFileName);
	vector<double> make256Level_0_1_Ramp();
	int useBlackWhiteColourRamp();
	int useHealthyColourRamp();
	int useInfectiousColourRamp();
	int useHealthyInfectiousColourRamp();
	int useDiscreteDistinctColourRamp(int nMinValue, int nMaxValue);

};
