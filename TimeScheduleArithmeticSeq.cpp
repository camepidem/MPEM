// TimeScheduleArithmeticSeq.cpp

#include "stdafx.h"
#include "TimeScheduleArithmeticSeq.h"

TimeScheduleArithmeticSeq::TimeScheduleArithmeticSeq(double timeFirst, double timeStep) {
	
	this->timeFirst = timeFirst;
	this->timeStep = timeStep;

	reset();

}

TimeScheduleArithmeticSeq::~TimeScheduleArithmeticSeq() {
	//
}

void TimeScheduleArithmeticSeq::advanceTimeNext() {

	timeNext = ScheduledTime(timeNext.isValidTime, timeNext.time + timeStep);

}

void TimeScheduleArithmeticSeq::reset() {

	timeNext = ScheduledTime(true, timeFirst);

}
