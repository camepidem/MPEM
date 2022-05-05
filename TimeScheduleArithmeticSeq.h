// TimeScheduleArithmeticSeq.h

#pragma once

#include "TimeSchedule.h"

class TimeScheduleArithmeticSeq : public TimeSchedule  {

public:

	TimeScheduleArithmeticSeq(double timeFirst, double timeStep);
	~TimeScheduleArithmeticSeq();

	void advanceTimeNext();

	void reset();

protected:

	double timeFirst;
	double timeStep;

};
