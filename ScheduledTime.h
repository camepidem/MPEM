// ScheduledTime.h

#pragma once

class ScheduledTime {

public:
	ScheduledTime() : ScheduledTime(false, 0.0) {};
	ScheduledTime(bool isValid, double timeScheduled) { isValidTime = isValid, time = timeScheduled; };

	bool isValidTime;
	double time;//TODO: Could enforce not being able to get at invalid times by having accessor function which throws an exception if an invalid time is asked for

};
