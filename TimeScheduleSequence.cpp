// TimeScheduleSequence.cpp

#include "stdafx.h"
#include "TimeScheduleSequence.h"

TimeScheduleSequence::TimeScheduleSequence(std::vector<double> times) {

	this->times = times;

	reset();

}

TimeScheduleSequence::~TimeScheduleSequence() {
	//
}

void TimeScheduleSequence::setTimeNext() {

	if (times.size() > currentTimeIndex) {
		timeNext = ScheduledTime(true, times[currentTimeIndex]);
	} else {
		timeNext = ScheduledTime(false, 0.0);
	}

}

void TimeScheduleSequence::advanceTimeNext() {

	currentTimeIndex++;

	setTimeNext();

}

void TimeScheduleSequence::reset() {

	currentTimeIndex = 0;

	setTimeNext();

}
