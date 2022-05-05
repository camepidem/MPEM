// TimeSchedule.h

#pragma once

#include "cfgutils.h"
#include "ScheduledTime.h"

class TimeSchedule {

public:

	TimeSchedule() : timeNext(false, 0.0) {};
	virtual ~TimeSchedule() {};

	ScheduledTime getTimeNext() { return timeNext; };

	void setTimeNextTo(double nextTime) { timeNext = ScheduledTime(true, nextTime); };
	void advanceTimeBy(double timeIncrement) { timeNext = ScheduledTime(timeNext.isValidTime, timeNext.time + timeIncrement); };

	virtual void advanceTimeNext() {};

	virtual void reset() {};

protected:

	ScheduledTime timeNext;

};
