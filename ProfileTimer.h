// ProfileTimer.h

#pragma once
//Intended use is to put a ProfileTimer on the stack, it will then automatically measure the run time of its lifetime in ms, 
//adding the value to targetClock on destruction or a call to stop() or lap()
class ProfileTimer {

public:

	ProfileTimer(int &targetClock, bool startRunning = true);
	~ProfileTimer();

	void start();//Set the timer going, automatically done at construction
	void stop();//manually stop the timer, automatically done at destruction. Increments the targetClock by elapsed time 

	void lap();//stop and re-start, increments targetClock by time elapsed since last stop() or lap()

protected:

	int &targetClock_;

	int startTime;

	bool isRunning;

};
