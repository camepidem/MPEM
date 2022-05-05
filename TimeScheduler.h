// TimeScheduler.h

#pragma once

#include <vector>
#include "ScheduledTime.h"

class TimeSchedule;

class TimeScheduler {

public:

	TimeScheduler();//Manual time schedule: before use, for every change and all resets must be explicitly specified with setTimeNextTo(time).
	TimeScheduler(double timeFirst, double timeStep);//Arithmetic sequence: starting at timeFirst and incrementing by timeStep on every call to advanceTimeNext()
	TimeScheduler(std::vector<double> times);//explicitly specified sequence of times, starting with times[0]. On running out of times, the SceduledTime s returned will be marked as invalid
	~TimeScheduler();

	ScheduledTime getTimeNext();

	void setTimeNextTo(double nextTime);
	void advanceTimeBy(double timeIncrement);

	void advanceTimeNext();

	void reset();

protected:

	TimeSchedule *schedule;

};
