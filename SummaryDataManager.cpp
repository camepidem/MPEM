//SummaryDataManager.cpp

#include "stdafx.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "ListManager.h"
#include "cfgutils.h"
#include "Raster.h"
#include "SummaryDataManager.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */
SummaryDataWeighted SummaryDataManager::weightedHostStates;
std::vector<SummaryDataZoning> SummaryDataManager::zonedHostStates;
std::string SummaryDataManager::zoneRasterFileName;
std::string SummaryDataManager::zoneRasterFileListFileName;
RasterHeader<int> SummaryDataManager::landscapeExtent;
vector<int> SummaryDataManager::publicZonings;

SummaryDataManager::SummaryDataManager()
{

}

SummaryDataManager::~SummaryDataManager()
{

}

int SummaryDataManager::init(RasterHeader<int> landscapeHeader) {

	setCurrentWorkingSubection("SummaryDataManager", CM_SIM);

	int bSuccess = 1;

	landscapeExtent = landscapeHeader;

	//Read weightings:
	int nWeightings = 0;

	if (readValueToProperty("NWeightings", &nWeightings, -1, ">=0") && nWeightings < 0) {
		reportReadError("ERROR: Specified number of weightings < 0\n");
		bSuccess = 0;
	}

	char szZoneFileName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szZoneFileName, "%s", "");
	if (readValueToProperty("ZoneRasterFileName", szZoneFileName, -1, "#FileName#")) {
		zoneRasterFileName = std::string(szZoneFileName);
	}

	char szZoneFileListFileName[_N_MAX_STATIC_BUFF_LEN];
	sprintf(szZoneFileListFileName, "%s", "");
	if (readValueToProperty("ZoneRasterFileListFileName", szZoneFileListFileName, -1, "#FileName#")) {
		zoneRasterFileListFileName = std::string(szZoneFileListFileName);
	}


	if (bSuccess && !bIsKeyMode()) {

		//Weightings:
		bSuccess *= weightedHostStates.init(nWeightings, landscapeHeader);

		//Zones:
		if (zoneRasterFileName != string("")) {
			int iPublicZone = SummaryDataManager::requestZoning(zoneRasterFileName);
			if (iPublicZone >= 0) {
				publicZonings.push_back(iPublicZone);
			}
		}

		if (zoneRasterFileListFileName != string("")) {

			std::ifstream zoneFS(zoneRasterFileListFileName, std::ifstream::in);

			std::string zoneRasterFileName;
			while (getline(zoneFS, zoneRasterFileName)) {

				trim(zoneRasterFileName);

				int iPublicZone = SummaryDataManager::requestZoning(zoneRasterFileName);
				if (iPublicZone >= 0) {
					publicZonings.push_back(iPublicZone);
				}

			}

			zoneFS.close();

		}
		
		zeroHostQuantities();

	}

	return bSuccess;

}

void SummaryDataManager::submitHostChange(int iPopType, int iClass, double dDeltaHostUnits, int xPos, int yPos) {

	weightedHostStates.submitHostChange(iPopType, iClass, dDeltaHostUnits, xPos, yPos);

	for (auto &zoning : zonedHostStates) {
		zoning.submitHostChange(iPopType, iClass, dDeltaHostUnits, xPos, yPos);
	}

	return;
}

std::vector<double> SummaryDataManager::getHostQuantities(int iPopType, int iWeighting) {
	return weightedHostStates.getHostQuantities(iPopType, iWeighting);
}

std::vector<double> SummaryDataManager::getHostQuantitiesZone(int iPopType, int iZone, int iZoneIndex) {
	return zonedHostStates[iZone].getHostQuantities(iPopType, iZoneIndex);
}

void SummaryDataManager::submitFirstInfection(int iPopType, int xPos, int yPos) {

	weightedHostStates.submitFirstInfection(iPopType, xPos, yPos);

	for (auto &zoning : zonedHostStates) {
		zoning.submitFirstInfection(iPopType, xPos, yPos);
	}

}

void SummaryDataManager::submitCurrentSusceptibles(int iPopType, int xPos, int yPos) {

	weightedHostStates.submitCurrentSusceptibles(iPopType, xPos, yPos);

	for (auto &zoning : zonedHostStates) {
		zoning.submitCurrentSusceptibles(iPopType, xPos, yPos);
	}

}

void SummaryDataManager::submitCurrentLossOfSusceptibles(int iPopType, int xPos, int yPos) {

	weightedHostStates.submitCurrentLossOfSusceptibles(iPopType, xPos, yPos);

	for (auto &zoning : zonedHostStates) {
		zoning.submitCurrentLossOfSusceptibles(iPopType, xPos, yPos);
	}

}

void SummaryDataManager::submitCurrentInfection(int iPopType, int xPos, int yPos) {

	weightedHostStates.submitCurrentInfection(iPopType, xPos, yPos);

	for (auto &zoning : zonedHostStates) {
		zoning.submitCurrentInfection(iPopType, xPos, yPos);
	}

}

void SummaryDataManager::submitCurrentLossOfInfection(int iPopType, int xPos, int yPos) {

	weightedHostStates.submitCurrentLossOfInfection(iPopType, xPos, yPos);

	for (auto &zoning : zonedHostStates) {
		zoning.submitCurrentLossOfInfection(iPopType, xPos, yPos);
	}

}

double SummaryDataManager::getUniqueLocationsInfected(int iWeighting) {
	return weightedHostStates.getUniqueLocationsInfected(iWeighting);
}

std::vector<double> SummaryDataManager::getUniquePopulationsInfected(int iWeighting) {
	return weightedHostStates.getUniquePopulationsInfected(iWeighting);
}

double SummaryDataManager::getUniqueLocationsInfectedZone(int iZoning, int iZoneIndex) {
	return zonedHostStates[iZoning].getUniqueLocationsInfected(iZoneIndex);
}

std::vector<double> SummaryDataManager::getUniquePopulationsInfectedZone(int iZoning, int iZoneIndex) {
	return zonedHostStates[iZoning].getUniquePopulationsInfected(iZoneIndex);
}

double SummaryDataManager::getCurrentLocationsInfected(int iWeighting) {
	return weightedHostStates.getCurrentLocationsInfected(iWeighting);
}

std::vector<double> SummaryDataManager::getCurrentPopulationsInfected(int iWeighting) {
	return weightedHostStates.getCurrentPopulationsInfected(iWeighting);
}

double SummaryDataManager::getCurrentLocationsInfectedZone(int iZoning, int iZoneIndex) {
	return zonedHostStates[iZoning].getCurrentLocationsInfected(iZoneIndex);
}

std::vector<double> SummaryDataManager::getCurrentPopulationsInfectedZone(int iZoning, int iZoneIndex) {
	return zonedHostStates[iZoning].getCurrentPopulationsInfected(iZoneIndex);
}

double SummaryDataManager::getCurrentLocationsSusceptible(int iWeighting) {
	return weightedHostStates.getCurrentLocationsSusceptible(iWeighting);
}

std::vector<double> SummaryDataManager::getCurrentPopulationsSusceptible(int iWeighting) {
	return weightedHostStates.getCurrentPopulationsSusceptible(iWeighting);
}

double SummaryDataManager::getCurrentLocationsSusceptibleZone(int iZoning, int iZoneIndex) {
	return zonedHostStates[iZoning].getCurrentLocationsSusceptible(iZoneIndex);
}

std::vector<double> SummaryDataManager::getCurrentPopulationsSusceptibleZone(int iZoning, int iZoneIndex) {
	return zonedHostStates[iZoning].getCurrentPopulationsSusceptible(iZoneIndex);
}

void SummaryDataManager::zeroHostQuantities() {

	weightedHostStates.zeroHostQuantities();

	for (auto &zoning : zonedHostStates) {
		zoning.zeroHostQuantities();
	}

}

void SummaryDataManager::resetUniqueInfections() {

	weightedHostStates.resetUniqueInfections();

	for (auto &zoning : zonedHostStates) {
		zoning.resetUniqueInfections();
	}

}

int SummaryDataManager::getNWeightings() {
	return weightedHostStates.getNWeightings();
}

double SummaryDataManager::getWeighting(int iWeight, int xPos, int yPos) {
	return weightedHostStates.getWeighting(iWeight, xPos, yPos);
}

int SummaryDataManager::requestZoning(std::string zoneRasterFilename) {

	Raster<int> zoneIDRaster;

	if (!(zoneRasterFilename == std::string(""))) {
		if (!zoneIDRaster.init(zoneRasterFilename)) {
			reportReadError("ERROR: Unable to read zone raster file: <<<%s>>>\n", zoneRasterFilename.c_str());
			return -1;
		}
		return requestZoning(zoneIDRaster);
	} else {
		reportReadError("ERROR: zone raster file name was empty string: \"%s\"\n", zoneRasterFilename.c_str());
		return -1;
	}

}

int SummaryDataManager::requestZoning(const Raster<int>& zoneIDRaster) {

	auto newZoning = SummaryDataZoning(zoneIDRaster, landscapeExtent);

	int validInit = newZoning.isValidZoning();
	if(validInit) {
		zonedHostStates.push_back(newZoning);
		return zonedHostStates.size() - 1; //Index of new zoning
	} else {
		return -1;
	}
}

int SummaryDataManager::getNZones(int iZoning) {
	return zonedHostStates[iZoning].getNZones();
}

int SummaryDataManager::getZoneIndex(int iZoning, int xPos, int yPos) {
	return zonedHostStates[iZoning].getZoneIndex(xPos, yPos);
}

int SummaryDataManager::getZoneID(int iZoning, int iZoneIndex) {
	return zonedHostStates[iZoning].getZoneID(iZoneIndex);
}

vector<int> SummaryDataManager::getPublicZonings() {
	return publicZonings;
}

SummaryState::SummaryState()
{

	pdHostQuantities.resize(PopulationCharacteristics::nPopulations);//For each class (inc. nEverRem) + nInfectious + nDetectable
	for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations; iPop++) {
		pdHostQuantities[iPop].resize(Population::pGlobalPopStats[iPop]->nTotalClasses);
	}
	pnUniquePopulationsInfected.resize(PopulationCharacteristics::nPopulations);
	pnCurrentPopulationsSusceptible.resize(PopulationCharacteristics::nPopulations);
	pnCurrentPopulationsInfected.resize(PopulationCharacteristics::nPopulations);

	zeroHostQuantities();
}

SummaryState::~SummaryState()
{
}

void SummaryState::zeroHostQuantities()
{

	for (int iPop = 0; iPop<PopulationCharacteristics::nPopulations; iPop++) {
		for (int iClass = 0; iClass<Population::pGlobalPopStats[iPop]->nTotalClasses; iClass++) {
			pdHostQuantities[iPop][iClass] = 0.0;
		}
	}

	nCurrentLocationsInfected    = 0;
	nCurrentLocationsSusceptible = 0;
	for (int iPop = 0; iPop<PopulationCharacteristics::nPopulations; iPop++) {
		pnCurrentPopulationsInfected[iPop]    = 0.0;
		pnCurrentPopulationsSusceptible[iPop] = 0.0;
	}

	resetUniqueInfections();

}

void SummaryState::resetUniqueInfections()
{
	nUniqueLocationsInfected = 0;

	for (int iPop = 0; iPop<PopulationCharacteristics::nPopulations; iPop++) {
		pnUniquePopulationsInfected[iPop] = 0;
	}

}

void SummaryState::submitHostChange(int iPopType, int iClass, double dDeltaHostUnits) {
	pdHostQuantities[iPopType][iClass] += dDeltaHostUnits;
}

void SummaryState::submitCurrentInfection(int iPopType, double dLocationCount) {
	if (iPopType >= 0) {
		pnCurrentPopulationsInfected[iPopType] += dLocationCount;
	} else {
		nCurrentLocationsInfected += dLocationCount;
	}
}

void SummaryState::submitCurrentLossOfInfection(int iPopType, double dLocationCount) {
	if (iPopType >= 0) {
		pnCurrentPopulationsInfected[iPopType] -= dLocationCount;
	} else {
		nCurrentLocationsInfected -= dLocationCount;
	}
}

void SummaryState::submitFirstInfection(int iPopType, double dLocationCount) {

	if (iPopType >= 0) {
		pnUniquePopulationsInfected[iPopType] += dLocationCount;
	} else {
		nUniqueLocationsInfected += dLocationCount;
	}

}

void SummaryState::submitCurrentSusceptibles(int iPopType, double dLocationCount) {
	if (iPopType >= 0) {
		pnCurrentPopulationsSusceptible[iPopType] += dLocationCount;
	} else {
		nCurrentLocationsSusceptible += dLocationCount;
	}
}

void SummaryState::submitCurrentLossOfSusceptibles(int iPopType, double dLocationCount) {
	if (iPopType >= 0) {
		pnCurrentPopulationsSusceptible[iPopType] -= dLocationCount;
	} else {
		nCurrentLocationsSusceptible -= dLocationCount;
	}
}

std::vector<double> SummaryState::getHostQuantities(int iPopType) {
	return pdHostQuantities[iPopType];
}

double SummaryState::getUniqueLocationsInfected() {
	return nUniqueLocationsInfected;
}

std::vector<double> SummaryState::getUniquePopulationsInfected() {
	return pnUniquePopulationsInfected;
}

double SummaryState::getCurrentLocationsInfected() {
	return nCurrentLocationsInfected;
}

std::vector<double> SummaryState::getCurrentPopulationsInfected() {
	return pnCurrentPopulationsInfected;
}

double SummaryState::getCurrentLocationsSusceptible() {
	return nCurrentLocationsSusceptible;
}

std::vector<double> SummaryState::getCurrentPopulationsSusceptible() {
	return pnCurrentPopulationsSusceptible;
}

SummaryDataZoning::SummaryDataZoning(const Raster<int>& zoneIDRaster, RasterHeader<int> landscapeHeader) {

	landscapeExtent = landscapeHeader;
	
	isValid = 1;

	if (!zoneIDRaster.header.sameAs(landscapeHeader)) {
		isValid = 0;
		reportReadError("ERROR: Zone raster does not have same extent as landscape\n");
	}

	if (!zoneRaster.init(zoneIDRaster)) {
		isValid = 0;
		reportReadError("ERROR: Zone raster cannot be initialised from specified raster\n");
	}

	//Get mapping of zones:
	for (int iY = 0; iY < zoneRaster.header.nRows; ++iY) {
		for (int iX = 0; iX < zoneRaster.header.nCols; ++iX) {
			int zoneID = getZone(iX, iY);
			//If zoneID is new, add it to the list:
			if (zonesIDToIndex.find(zoneID) == zonesIDToIndex.end()) {
				int newIndex = zonesIDToIndex.size();
				zonesIDToIndex[zoneID] = newIndex;
				zonesIndexToID[newIndex] = zoneID;
			}
		}
	}

	hostStates.resize(zonesIDToIndex.size());

}

SummaryDataZoning::~SummaryDataZoning() {
}

int SummaryDataZoning::isValidZoning() {
	return isValid;
}

void SummaryDataZoning::submitHostChange(int iPopType, int iClass, double dDeltaHostUnits, int xPos, int yPos) {
	hostStates[getZoneIndex(xPos, yPos)].submitHostChange(iPopType, iClass, dDeltaHostUnits);
}

void SummaryDataZoning::submitCurrentInfection(int iPopType, int xPos, int yPos) {
	hostStates[getZoneIndex(xPos, yPos)].submitCurrentInfection(iPopType);
}

void SummaryDataZoning::submitCurrentLossOfInfection(int iPopType, int xPos, int yPos) {
	hostStates[getZoneIndex(xPos, yPos)].submitCurrentLossOfInfection(iPopType);
}

void SummaryDataZoning::submitFirstInfection(int iPopType, int xPos, int yPos) {
	hostStates[getZoneIndex(xPos, yPos)].submitFirstInfection(iPopType);
}

void SummaryDataZoning::submitCurrentSusceptibles(int iPopType, int xPos, int yPos) {
	hostStates[getZoneIndex(xPos, yPos)].submitCurrentSusceptibles(iPopType);
}

void SummaryDataZoning::submitCurrentLossOfSusceptibles(int iPopType, int xPos, int yPos) {
	hostStates[getZoneIndex(xPos, yPos)].submitCurrentLossOfSusceptibles(iPopType);
}

std::vector<double> SummaryDataZoning::getHostQuantities(int iPopType, int iZoneIndex) {
	return hostStates[iZoneIndex].getHostQuantities(iPopType);
}

double SummaryDataZoning::getUniqueLocationsInfected(int iZoneIndex) {
	return hostStates[iZoneIndex].getUniqueLocationsInfected();
}

std::vector<double> SummaryDataZoning::getUniquePopulationsInfected(int iZoneIndex) {
	return hostStates[iZoneIndex].getUniquePopulationsInfected();
}

double SummaryDataZoning::getCurrentLocationsInfected(int iZoneIndex) {
	return hostStates[iZoneIndex].getCurrentLocationsInfected();
}

std::vector<double> SummaryDataZoning::getCurrentPopulationsInfected(int iZoneIndex) {
	return hostStates[iZoneIndex].getCurrentPopulationsInfected();
}

double SummaryDataZoning::getCurrentLocationsSusceptible(int iZoneIndex) {
	return hostStates[iZoneIndex].getCurrentLocationsSusceptible();
}

std::vector<double> SummaryDataZoning::getCurrentPopulationsSusceptible(int iZoneIndex) {
	return hostStates[iZoneIndex].getCurrentPopulationsSusceptible();
}

void SummaryDataZoning::zeroHostQuantities() {
	for (auto &hostState : hostStates) {
		hostState.zeroHostQuantities();
	}
}

void SummaryDataZoning::resetUniqueInfections() {
	for (auto &hostState : hostStates) {
		hostState.resetUniqueInfections();
	}
}

int SummaryDataZoning::getNZones() {
	return zonesIDToIndex.size();
}

int SummaryDataZoning::getZoneIndex(int xPos, int yPos) {
	int zoneID = zoneRaster(xPos, yPos);
	if (zonesIDToIndex.find(zoneID) != zonesIDToIndex.end()) {
		return zonesIDToIndex[zoneID];
	}
	return -1;
}

int SummaryDataZoning::getZoneID(int iZoneIndex) {
	if (zonesIndexToID.find(iZoneIndex) != zonesIndexToID.end()) {
		return zonesIndexToID[iZoneIndex];
	}
	return -1234567890;
}

int SummaryDataZoning::getZone(int xPos, int yPos) {
	return zoneRaster(xPos, yPos);
}

SummaryDataWeighted::SummaryDataWeighted() {

}

SummaryDataWeighted::~SummaryDataWeighted() {
}

int SummaryDataWeighted::init(int nWeightings, RasterHeader<int> landscapeHeader) {

	landscapeExtent = landscapeHeader;

	//Read all necessary weightings files:
	int bWeightInitSuccess = 1;
	if (nWeightings > 0) {
		weightings.resize(nWeightings);
		for (int iWeight = 0; iWeight < nWeightings; iWeight++) {
			char szWeightRasterName[_N_MAX_STATIC_BUFF_LEN];
			sprintf(szWeightRasterName, "L_WEIGHTRASTER_%d.txt", iWeight);
			bWeightInitSuccess *= weightings[iWeight].init(std::string(szWeightRasterName));

			if (!weightings[iWeight].header.sameAs(landscapeExtent)) {
				reportReadError("ERROR: Weighting file %s does not have same extent as the landscape\n", szWeightRasterName);
				bWeightInitSuccess = 0;
			}

			if (bWeightInitSuccess) {
				//Ensure no negative weightings:
				int bFoundNegatives = 0;
				for (int x = 0; x < weightings[iWeight].header.nCols; x++) {
					for (int y = 0; y < weightings[iWeight].header.nRows; y++) {
						if (weightings[iWeight](x, y) < 0.0) {
							weightings[iWeight](x, y) = 0.0;
							bFoundNegatives = 1;
						}
					}
				}

				if (bFoundNegatives) {
					printk("Warning: Weighting file %s has negative values, setting all negative values to 0.0.\n", szWeightRasterName);
				}
			}

		}

		if (!bWeightInitSuccess) {
			reportReadError("ERROR: Unable to successfully read weighting files\n");
		}
	}

	nWeightings++;//Additionally have the default flat weighting as #0

	hostStates.resize(nWeightings);

	return bWeightInitSuccess;
}

void SummaryDataWeighted::submitHostChange(int iPopType, int iClass, double dDeltaHostUnits, int xPos, int yPos) {
	for (size_t iWeight = 0; iWeight < hostStates.size(); iWeight++) {
		hostStates[iWeight].submitHostChange(iPopType, iClass, dDeltaHostUnits*getWeighting(iWeight, xPos, yPos));
	}
}

void SummaryDataWeighted::submitCurrentInfection(int iPopType, int xPos, int yPos) {
	for (size_t iWeight = 0; iWeight < hostStates.size(); iWeight++) {
		hostStates[iWeight].submitCurrentInfection(iPopType, getWeighting(iWeight, xPos, yPos));
	}
}

void SummaryDataWeighted::submitCurrentLossOfInfection(int iPopType, int xPos, int yPos) {
	for (size_t iWeight = 0; iWeight < hostStates.size(); iWeight++) {
		hostStates[iWeight].submitCurrentLossOfInfection(iPopType, getWeighting(iWeight, xPos, yPos));
	}
}

void SummaryDataWeighted::submitFirstInfection(int iPopType, int xPos, int yPos) {
	for (size_t iWeight = 0; iWeight < hostStates.size(); iWeight++) {
		hostStates[iWeight].submitFirstInfection(iPopType, getWeighting(iWeight, xPos, yPos));
	}
}

void SummaryDataWeighted::submitCurrentSusceptibles(int iPopType, int xPos, int yPos) {
	for (size_t iWeight = 0; iWeight < hostStates.size(); iWeight++) {
		hostStates[iWeight].submitCurrentSusceptibles(iPopType, getWeighting(iWeight, xPos, yPos));
	}
}

void SummaryDataWeighted::submitCurrentLossOfSusceptibles(int iPopType, int xPos, int yPos) {
	for (size_t iWeight = 0; iWeight < hostStates.size(); iWeight++) {
		hostStates[iWeight].submitCurrentLossOfSusceptibles(iPopType, getWeighting(iWeight, xPos, yPos));
	}
}

std::vector<double> SummaryDataWeighted::getHostQuantities(int iPopType, int iWeighting) {
	return hostStates[iWeighting].getHostQuantities(iPopType);
}

double SummaryDataWeighted::getUniqueLocationsInfected(int iWeighting) {
	return hostStates[iWeighting].getUniqueLocationsInfected();
}

std::vector<double> SummaryDataWeighted::getUniquePopulationsInfected(int iWeighting) {
	return hostStates[iWeighting].getUniquePopulationsInfected();
}

double SummaryDataWeighted::getCurrentLocationsInfected(int iWeighting) {
	return hostStates[iWeighting].getCurrentLocationsInfected();
}

std::vector<double> SummaryDataWeighted::getCurrentPopulationsInfected(int iWeighting) {
	return hostStates[iWeighting].getCurrentPopulationsInfected();
}

double SummaryDataWeighted::getCurrentLocationsSusceptible(int iWeighting) {
	return hostStates[iWeighting].getCurrentLocationsSusceptible();
}

std::vector<double> SummaryDataWeighted::getCurrentPopulationsSusceptible(int iWeighting) {
	return hostStates[iWeighting].getCurrentPopulationsSusceptible();
}

void SummaryDataWeighted::zeroHostQuantities() {
	for (auto &hostState : hostStates) {
		hostState.zeroHostQuantities();
	}
}

void SummaryDataWeighted::resetUniqueInfections() {
	for (auto &hostState : hostStates) {
		hostState.resetUniqueInfections();
	}
}

int SummaryDataWeighted::getNWeightings() {
	return hostStates.size();
}

double SummaryDataWeighted::getWeighting(int iWeight, int xPos, int yPos) {
	if (iWeight == 0) {
		return 1.0;
	} else {
		return weightings[iWeight - 1](xPos, yPos);
	}
}
