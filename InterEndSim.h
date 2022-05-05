//InterEndSim.h

//Contains the data for the End Simulation Intervention

#ifndef INTERENDSIM_H
#define INTERENDSIM_H 1

#include "Intervention.h"
#include <thread>

struct EndSimCondition {

	double time;
	std::string statistic;
	std::string metric;
	std::string targetData;
	std::string comparator;
	double value;

};

class TargetData;

class InterEndSim : public Intervention {

public:

	InterEndSim();

	~InterEndSim();

	bool disableEndAtTimeCondition();

	double getTimeNext();

	int setTimeNext(double dTimeNext);

	bool bEarlyStoppingConditionsEnabled();

protected:
	
	int intervene();
	int finalAction();
	
	double findNextTime(double dCurrentTime);

	bool bEndAtTimeCondition;

	//Early termination constraints:
	std::vector<EndSimCondition> readConditionsFromFile(std::string fileName);

	std::vector<EndSimCondition> conditions;

	bool testConstraint(EndSimCondition constraint);

	std::map<std::string, TargetData *> targetData;

	//Wallclock time killing:
	std::thread wallclockAbortThread;

};

#endif
