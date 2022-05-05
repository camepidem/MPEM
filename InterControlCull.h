//InterControlCull.h

//Contains the data for the Output Data Intervention

#ifndef INTERCONTROLCULL_H
#define INTERCONTROLCULL_H 1

#include "Intervention.h"

struct sLocPriorityPair {
	LocationMultiHost *pLoc;
	double dPriority;
};

//TODO: Make control interventions general, what to do when controlling determined by switch within intervention
//TODO: How to instantiate an arbitrary number of these guys?
class InterControlCull : public Intervention {

public:

	InterControlCull();

	~InterControlCull();

	int intervene();

	int finalAction();

	double totalCost;

	//Should detection state of location be forgotten after an application of control?
	int forgets;

	int iPopulationToControl;

	void writeSummaryData(FILE *pFile);

	enum ControlType { CT_NONE, CT_CULL, CT_ROGUE, CT_SPRAYAS, CT_SPRAYPR, CT_SPRAYBOTH, CT_MAX };//TODO: can sprays be rolled into this - probably, just need to wrapper output at end function...?

protected:

	//Dynamic Control:
	double timeNextDynamicControl;
	double timeLast;

	//Types of control that can be effected:
	static const char *ControlTypeNames[];

	ControlType ctNameToCT(const char *szControlName);
	static const char *szCTToName(ControlType ctType);

	//What control to apply where:
	enum ControlMethodSpecification {CMS_BLANKET, CMS_RASTER, CMS_MAX};//TODO: Other forms cf sam work?
	static const char *ControlMethodSpecificationNames[];
	ControlMethodSpecification cmsControlMethod;

	ControlMethodSpecification cmsNameToCMS(const char *szControlMethodSpecificationName);
	static const char *szCMSToName(ControlMethodSpecification cmsSpec);

	friend std::stringstream& operator>>(std::stringstream& strStream, InterControlCull::ControlMethodSpecification& tempEnum) {

		int iTemp;

		strStream >> iTemp;

		tempEnum = (InterControlCull::ControlMethodSpecification)iTemp;

		return strStream;

	}

	//For CM_BLANKET:
	char *szCM_BlanketName;
	ControlType ctControlMethodBlanketType;

	//For CM_RASTER:
	char *szCM_RasterName;
	Raster<ControlType> *prcmCM_Raster;
	//TODO: how to read this in? text?

	//How hard to apply control measures:
	enum ControlIntensitySpecification {CIS_BLANKET, CIS_FROMRASTER, CIS_MAX};//TODO: Other forms cf sam work
	static const char *ControlIntensitySpecificationNames[];
	ControlIntensitySpecification cisControlIntensity;
	
	ControlIntensitySpecification cisNameToCIS(char *szControlIntenstiySpecificationName);
	static const char *szCISToName(ControlIntensitySpecification cisSpec);

	//For CI_BLANKET:
	double dCI_BlanketRadius;

	//For CI_FROMRASTER:
	char *szCI_RasterName;
	Raster<double> *prCI_RasterRadius;

	//Interface for determining what to do where:
	double dGetControlRadius(LocationMultiHost *pLocationCentre);
	ControlType ctGetControlType(LocationMultiHost *pLocationCentre);

	double effectiveness;//TODO: Also able to specifty this on a per location basis?

	//Decaying spraying:
	//Spray properties:
	struct SprayProperties {
		int decays;
		double decayPeriod;
		double decayFactor;
		double minEffectiveness;
	};

	SprayProperties sprayASProps;
	SprayProperties sprayPRProps;
	
	struct SprayEffect {
		SprayEffect() : currentEffectiveness(0.0), dMaxAllowedHost(1.0), timeNextDecay(0.0) {};
		SprayEffect(double currentEffectiveness, double dMaxAllowedHost, double timeNextDecay) : currentEffectiveness(currentEffectiveness), dMaxAllowedHost(dMaxAllowedHost), timeNextDecay(timeNextDecay) {};
		double currentEffectiveness;
		double dMaxAllowedHost;
		double timeNextDecay;
	};

	std::map<LocationMultiHost*, SprayEffect> activeSprayAS;
	std::map<LocationMultiHost*, SprayEffect> activeSprayPR;

	void updateSprayStatus();

	double timeNextSprayDecay;

	//Scripted controls:
	int bUseScriptedControls;

	double timeNextScriptedControl;
	double timeDoneScriptedControl;

	void applyScriptedControls();

	struct ScriptedControlData {
		double applicationTime;
		ControlType type;
		double effectiveness;
		int targetPopulation;
		std::vector<LocationMultiHost*> locations;
	};

	std::vector<ScriptedControlData> scriptedControls;

	std::string scriptedControlFileName;

	//Apply control to a location -possibly due to relationship to another location (centre). Returns quantity of work done
	double dControlLocation(LocationMultiHost *pLocationToActuallyApplyControl, LocationMultiHost *pLocationCentre, ControlType controlType, int iTargetPopulation, double dEffectiveness, double dMaxAllowedHost);


	//Determine whether or not somewhere should be controlled, returns priority > 0.0 indicates control desired:
	double dShouldControlLocation(LocationMultiHost *pLocationTest);

	vector<sLocPriorityPair> psLocationPriorityOrder;

	//Functions for determining control priority
	int bPriorityBasedControl;
	double dTotalBudgetPerSweep;
	double dCostPerVisit;
	double dCostPerUnitHost;

	//TODO: Control based on cumulative priority of contiguous region AND remove region at once...
	//TODO: able to assess/estimate cost of doing a control before doing it to prevent budget overruns

	enum PrioritySystems {PRI_RANDOM, PRI_RASTER, PRI_MAX};//TODO: Other forms cf sam work
	PrioritySystems iPrioritySystem;

	//Input rasters for control priority
	char *szPriorityRasterName;
	Raster<double> *prPriorities;

	int reset();

};

#endif
