// TimeScheduler.cpp

#include "stdafx.h"

#include "TimeSchedule.h"
#include "TimeScheduleArithmeticSeq.h"
#include "TimeScheduleSequence.h"

#include "TimeScheduler.h"

TimeScheduler::TimeScheduler() {
	schedule = new TimeSchedule();
}

TimeScheduler::TimeScheduler(double timeFirst, double timeStep) {
	schedule = new TimeScheduleArithmeticSeq(timeFirst, timeStep);
}

TimeScheduler::TimeScheduler(std::vector<double> times) {
	schedule = new TimeScheduleSequence(times);
}

TimeScheduler::~TimeScheduler() {
	delete schedule;
}

ScheduledTime TimeScheduler::getTimeNext() {
	return schedule->getTimeNext();
}

void TimeScheduler::setTimeNextTo(double nextTime) {
	schedule->setTimeNextTo(nextTime);
}

void TimeScheduler::advanceTimeBy(double timeIncrement) {
	schedule->advanceTimeBy(timeIncrement);
}


void TimeScheduler::advanceTimeNext() {
	schedule->advanceTimeNext();
}


void TimeScheduler::reset() {
	schedule->reset();
}
