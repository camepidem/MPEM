// TimeScheduleSequence.h

#pragma once

#include <vector>
#include "TimeSchedule.h"

class TimeScheduleSequence : public TimeSchedule  {

public:

	TimeScheduleSequence(std::vector<double> times);
	~TimeScheduleSequence();

	void advanceTimeNext();

	void reset();

protected:

	void setTimeNext();

	std::vector<double> times;
	size_t currentTimeIndex;

};
