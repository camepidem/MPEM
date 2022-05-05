//RateStructure.h

#ifndef RATESTRUCTURE_H
#define RATESTRUCTURE_H 1

class RateStructure {

public:

	//Constructor/destructor:
	RateStructure();
	virtual ~RateStructure();

	//Interface methods:
	
	//Rate reporting:
	//Location submits rate:
	virtual double submitRate(int locNo, double rate);
	//Retrieve rate for given location:
	virtual double getRate(int locNo);

	//Find total rate of event:
	virtual double getTotalRate();

	//Find location for given marginal rate 
	virtual int getLocationEventIndex(double rate);

	//cleans out all rates to 0.0 (part of interventions)
	virtual int scrubRates();												

	//Find approx number of operations to resubmit all rates (used when landscape needs to be cleared; may be easier to just manually set all rates to 0.0)
	virtual int getWorkToResubmit();

	//Input to structure automatically summing, useful for resubmitting large portion of rate structure, must be followed by a fullResum() before structure is used
	virtual double submitRate_dumb(int locNo, double rate);

	//Forces recalculation of whole rate structure:
	virtual int fullResum();

protected:
	
};

#endif
