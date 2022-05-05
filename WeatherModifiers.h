//WeatherModifiers.h

#pragma once

#include "cfgutils.h"

enum WeatherModifiers {SUSCEPTIBILITY, INFECTIVITY, MODIFIERS_LENGTH};

class PopModPair {

public:

	PopModPair(int iPop_=0, WeatherModifiers wmMod_=INFECTIVITY) : iPop(iPop_), wmMod(wmMod_) {};
	PopModPair(string sPopMod) {
		vector<string> vPair;
		split(vPair, sPopMod, "_");


		stringstream ssPop(vPair[0]);
		if(!(ssPop >> iPop)) {
			reportReadError("Unable to parse population from weather file target (POP_MODIFIER) string was %s\n", sPopMod.c_str());
		}

		int nFound = 0;
		if(vPair[1].find_first_of("Ss") == 0) {
			nFound++;
			wmMod = SUSCEPTIBILITY;
		}
		if(vPair[1].find_first_of("Ii") == 0) {
			nFound++;
			wmMod = INFECTIVITY;
		}

		if(nFound != 1) {
			reportReadError("Unable to parse SUSCEPTIBILITY or INFECTIVITY file target (POP_MODIFIER) string was %s\n", sPopMod.c_str());
		}

	}

	int iPop;
	WeatherModifiers wmMod;

};
