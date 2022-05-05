//Intervention.h

//This object defines the properties and interface of an Intervention

#ifndef INTERVENTION_H
#define INTERVENTION_H 1

#include "classes.h"

class Intervention {
	
public:
	
	Intervention();
	virtual ~Intervention();

	int performIntervention();

	int performReset();

	int performFinalAction();

	virtual int disable();

	virtual void writeSummaryData(FILE *pFile);

	virtual double getTimeNext();

	virtual int execute(int iArg);//For external interventions this function is called by the RateManager when the stochastic event is executed

	int timeSpent;

	char *InterName;

	int enabled;

	//virtual int setTimeFirst();

	static int setWorld(Landscape *myWorld);

protected:

	static Landscape *world;

	//Actual functionality:
	virtual int intervene();
	virtual int reset();
	virtual int finalAction();

	//Sequencing properties
	double frequency;
	double timeFirst;

	double timeNext;

};


#endif
