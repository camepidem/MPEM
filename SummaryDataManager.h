//SummaryDataManager.h

#pragma once

#include <unordered_map>
#include "Raster.h"

class SummaryState
{
public:
	SummaryState();
	~SummaryState();

	void zeroHostQuantities();
	void resetUniqueInfections();

	void submitHostChange(int iPopType, int iClass, double dDeltaHostUnits);

	//Tracking of locations/populations that currently have some number of epidemiologically active hosts (ie not S or R)
	void submitCurrentInfection(int iPopType, double dLocationCount = 1.0);
	void submitCurrentLossOfInfection(int iPopType, double dLocationCount = 1.0);
	void submitFirstInfection(int iPopType, double dLocationCount = 1.0);
	void submitCurrentSusceptibles(int iPopType, double dLocationCount = 1.0);
	void submitCurrentLossOfSusceptibles(int iPopType, double dLocationCount = 1.0);

	std::vector<double> getHostQuantities(int iPopType);

	double getUniqueLocationsInfected();
	std::vector<double> getUniquePopulationsInfected();

	double getCurrentLocationsInfected();
	std::vector<double> getCurrentPopulationsInfected();

	double getCurrentLocationsSusceptible();
	std::vector<double> getCurrentPopulationsSusceptible();

protected:
	std::vector<std::vector<double>> pdHostQuantities;//[iPop][iClass]

	double			nUniqueLocationsInfected;
	std::vector<double>	pnUniquePopulationsInfected;//[iPop]

	double			nCurrentLocationsInfected;
	std::vector<double>	pnCurrentPopulationsInfected;//[iPop]

	double          nCurrentLocationsSusceptible;
	std::vector<double> pnCurrentPopulationsSusceptible;//[iPop]

};

class SummaryDataZoning {
public:
	SummaryDataZoning(const Raster<int> &zoneIDRasterint, RasterHeader<int> landscapeHeader);
	~SummaryDataZoning();

	int isValidZoning();

	void submitHostChange(int iPopType, int iClass, double dDeltaHostUnits, int xPos, int yPos);

	//Tracking of locations/populations that currently have some number of epidemiologically active hosts (ie not S or R)
	void submitCurrentInfection(int iPopType, int xPos, int yPos);
	void submitCurrentLossOfInfection(int iPopType, int xPos, int yPos);
	void submitFirstInfection(int iPopType, int xPos, int yPos);
	void submitCurrentSusceptibles(int iPopType, int xPos, int yPos);
	void submitCurrentLossOfSusceptibles(int iPopType, int xPos, int yPos);

	std::vector<double> getHostQuantities(int iPopType, int iZoneIndex);

	double getUniqueLocationsInfected(int iZoneIndex);
	std::vector<double> getUniquePopulationsInfected(int iZoneIndex);

	double getCurrentLocationsInfected(int iZoneIndex);
	std::vector<double> getCurrentPopulationsInfected(int iZoneIndex);

	double getCurrentLocationsSusceptible(int iZoneIndex);
	std::vector<double> getCurrentPopulationsSusceptible(int iZoneIndex);

	void zeroHostQuantities();

	void resetUniqueInfections();

	int getNZones();
	int getZoneIndex(int xPos, int yPos);
	int getZoneID(int iZoneIndex);

protected:
	std::vector<SummaryState> hostStates;

	std::unordered_map<int, int> zonesIndexToID; //Maps from internal zone index to zone ID specified in the zoneRaster
	std::unordered_map<int, int> zonesIDToIndex; //Maps from internal zone index to zone ID specified in the zoneRaster
	Raster<int> zoneRaster;
	int getZone(int xPos, int yPos);

	RasterHeader<int> landscapeExtent;

	int isValid;

};

class SummaryDataWeighted {
public:
	SummaryDataWeighted();
	~SummaryDataWeighted();

	int init(int nWeightings, RasterHeader<int> landscapeHeader);

	void submitHostChange(int iPopType, int iClass, double dDeltaHostUnits, int xPos, int yPos);

	//Tracking of locations/populations that currently have some number of epidemiologically active hosts (ie not S or R)
	void submitCurrentInfection(int iPopType, int xPos, int yPos);
	void submitCurrentLossOfInfection(int iPopType, int xPos, int yPos);
	void submitFirstInfection(int iPopType, int xPos, int yPos);
	void submitCurrentSusceptibles(int iPopType, int xPos, int yPos);
	void submitCurrentLossOfSusceptibles(int iPopType, int xPos, int yPos);

	std::vector<double> getHostQuantities(int iPopType, int iWeighting = 0);

	double getUniqueLocationsInfected(int iWeighting = 0);
	std::vector<double> getUniquePopulationsInfected(int iWeighting = 0);

	double getCurrentLocationsInfected(int iWeighting = 0);
	std::vector<double> getCurrentPopulationsInfected(int iWeighting = 0);

	double getCurrentLocationsSusceptible(int iWeighting = 0);
	std::vector<double> getCurrentPopulationsSusceptible(int iWeighting = 0);

	void zeroHostQuantities();

	void resetUniqueInfections();

	int getNWeightings();
	double getWeighting(int iWeight, int xPos, int yPos);

protected:
	//by weight and population: [iWeight][iPop][iClass]
	std::vector<SummaryState> hostStates;

	vector<Raster<double>> weightings;

	RasterHeader<int> landscapeExtent;

};

class SummaryDataManager
{

public:
	
	SummaryDataManager();
	~SummaryDataManager();

	static int init(RasterHeader<int> landscapeHeader);

	static void submitHostChange(int iPopType, int iClass, double dDeltaHostUnits, int xPos, int yPos);

	//Tracking of locations/populations that currently have some number of epidemiologically active hosts (ie not S or R)
	static void submitCurrentInfection(int iPopType, int xPos, int yPos);
	static void submitCurrentLossOfInfection(int iPopType, int xPos, int yPos);
	static void submitFirstInfection(int iPopType, int xPos, int yPos);
	static void submitCurrentSusceptibles(int iPopType, int xPos, int yPos);
	static void submitCurrentLossOfSusceptibles(int iPopType, int xPos, int yPos);

	//Returns the quantity of hosts in each class in a given population
	static std::vector<double> getHostQuantities(int iPopType, int iWeighting=0);
	static std::vector<double> getHostQuantitiesZone(int iPopType, int iZoning, int iZoneIndex);

	static double getUniqueLocationsInfected(int iWeighting=0);
	static std::vector<double> getUniquePopulationsInfected(int iWeighting=0);
	static double getUniqueLocationsInfectedZone(int iZoning, int iZoneIndex);
	static std::vector<double> getUniquePopulationsInfectedZone(int iZoning, int iZoneIndex);

	static double getCurrentLocationsInfected(int iWeighting=0);
	static std::vector<double> getCurrentPopulationsInfected(int iWeighting=0);
	static double getCurrentLocationsInfectedZone(int iZoning, int iZoneIndex);
	static std::vector<double> getCurrentPopulationsInfectedZone(int iZoning, int iZoneIndex);

	static double getCurrentLocationsSusceptible(int iWeighting = 0);
	static std::vector<double> getCurrentPopulationsSusceptible(int iWeighting = 0);
	static double getCurrentLocationsSusceptibleZone(int iZoning, int iZoneIndex);
	static std::vector<double> getCurrentPopulationsSusceptibleZone(int iZoning, int iZoneIndex);

	//Set all host data to zero (clean for first submission of data):
	static void zeroHostQuantities();
	
	static void resetUniqueInfections();

	static int getNWeightings();
	static double getWeighting(int iWeight, int xPos, int yPos);

	static int requestZoning(std::string zoneRasterFilename);
	static int requestZoning(const Raster<int> &zoneIDRaster);

	static int getNZones(int iZoning);
	static int getZoneIndex(int iZoning, int xPos, int yPos);
	static int getZoneID(int iZoning, int iZoneIndex);

	static vector<int> getPublicZonings();

protected:

	static RasterHeader<int> landscapeExtent;

	//Host counts:
	static SummaryDataWeighted weightedHostStates;
	static std::vector<SummaryDataZoning> zonedHostStates;

	static std::string zoneRasterFileName;
	static std::string zoneRasterFileListFileName;

	static vector<int> publicZonings;//indices of zones that were publicly requested, and will be output for DPCs etc

};
