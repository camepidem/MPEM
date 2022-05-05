//classes.h

#pragma once

//Forward declaration of all classes

//Simulation Classes
class LocationMultiHost;

class Population;
class PopulationCharacteristics;

class Landscape;

//Events:
class EventManager;

//Rates:
class RateManager;

class RateStructure;
class RateCompositionRejection;
class RateCompositionGroup;
class RateTree;
class RateTreeInt;
class RateIntervals;
class RateSum;

//Dispersal:
class DispersalManager;

class DispersalKernel;
class DispersalKernelGeneric;
class DispersalKernelRasterBased;
class DispersalKernelFunctionalUSSOD;

class KernelRaster;
class KernelRasterArray;
class KernelRasterPoints;



//Lists:
class ListManager;

template <class T> class LinkedList;
template <class T> struct ListNode;

//Data structures
template <class T> class Raster;
template <class T> class RasterHeader;
template <class T> class PointXYV;

class ColourRamp;

class ProfileTimer;
class SimulationProfiler;

class InoculumTrap;

class SpatialStatistic;
class SpatialStatisticCounts;
class SpatialStatisticRing;
class SpatialStatisticMoranI;

class ClimateScheme;
class ClimateSchemeMarkov;
class ClimateSchemeScript;

class CohortTransitionScheme;
class CohortTransitionSchemeScript;
class CohortTransitionSchemePeriodic;

class ClimateSchemeScriptKernel;
class ClimateSchemeScriptDouble;

class ParameterDistribution;
class ParameterDistributionUniform;
class ParameterDistributionPosterior;
class CParameterPosterior;

//Summary Data:
class SummaryDataManager;

//Intervention Classes
class InterventionManager;

class Intervention;

class InterEndSim;
class InterOutputDataDPC;
class InterOutputDataRaster;
class InterSpatialStatistics;
class InterVideo;
class InterDetectionSweep;
class InterControlCull;
class InterControlRogue;
class InterControlSprayAS;
class InterControlSprayPR;
class InterClimate;
class InterCohortTransition;
class InterVariableBackgroundInfection;
class InterWeather;
class InterHurricane;
class InterInitialInf;
class InterScriptedInfectionData;
class InterParameterDistribution;
class InterRiskMap;
class InterR0Map;
class InterThreatMap;
class InterBatch;

struct EventInfo {

	LocationMultiHost *pLoc;	//Location of last event to occur -can determine update region
	int iPop;					//Population type of last event to occur -can determine update region
	int bRanged;				//if last event type was long ranged -use for determining rate update needs
	double dDeltaInfectiousness;//holds a quantity from lastEventLoc so can quickly subtract lastEventLoc's previous contribution
};
