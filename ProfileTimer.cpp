// ProfileTimer.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "ProfileTimer.h"


ProfileTimer::ProfileTimer(int &targetClock, bool startRunning) : targetClock_(targetClock), isRunning(false) {

	if (startRunning) {
		start();
	}

	isRunning = startRunning;

}

ProfileTimer::~ProfileTimer() {

	stop();

}

void ProfileTimer::start() {

	if (!isRunning) {
		isRunning = true;
		startTime = clock_ms();
	}

}

void ProfileTimer::stop() {

	if (isRunning) {

		isRunning = false;

		targetClock_ += clock_ms() - startTime;

	}

}
