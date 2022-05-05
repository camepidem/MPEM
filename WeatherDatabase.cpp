//WeatherDatabase.cpp

#include "stdafx.h"
#include "Raster.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "cfgutils.h"
#include "WeatherDatabase.h"

WeatherDatabase::WeatherDatabase() {

}

WeatherDatabase::~WeatherDatabase() {

}

int WeatherDatabase::init(Landscape *pWorld) {

	world = pWorld;

	cout << "Reading weather files:" << endl;
	size_t nFiles = mapIDtoRasters.size();
	int nRead = 0;

	//Fake LandscapeRaster for raster translation
	Raster<int> landscapeRaster(1,1, world->header->xllCorner, world->header->yllCorner, world->header->cellsize, 0);
	landscapeRaster.header.nCols = world->header->nCols;
	landscapeRaster.header.nRows = world->header->nRows;

	for(auto it = mapIDtoRasters.begin(); it != mapIDtoRasters.end(); ++it) {
		
		string weatherFileName = it->first;
		Raster<double> &weatherRaster = it->second;
		
		if(weatherRaster.init(weatherFileName)) {
			nRead++;
			std::cout << "Read " << nRead << "/" << nFiles << " FileName=\"" << weatherFileName << "\"" << endl;

			//Align the raster to the landscape:
		

			double dCellRatio = weatherRaster.header.cellsize/world->header->cellsize;
			int iCellRatio = int(floor(dCellRatio + 0.5));

			if(abs(dCellRatio - iCellRatio) > 1e-3) {
				reportReadError("Weather files are required to have cellsizes that are integer multiples of each other, ratio: %f\n", dCellRatio);
			}

			if(iCellRatio < 1) {
				reportReadError("Weather files are required to have cellsizes (%f) that are >= cellsize of the landscape (%f)\n", weatherRaster.header.cellsize, world->header->cellsize);
			}

			//Ensure that cellsize is the ratio we think it is:
			weatherRaster.header.cellsize = world->header->cellsize*iCellRatio;
		
			//Tweak header so that weather raster lies on landscape grid:
			//Find containing cell in landscape raster of LLC in weather raster
			vector<int> landscapeCell = landscapeRaster.realToCell(weatherRaster.header.xllCorner, weatherRaster.header.yllCorner);

			//Find LLC of that cell
			vector<double> landscapeCoord = landscapeRaster.cellToReal(landscapeCell);

			//translate weather raster LLC so that it lies on a corner the cell containing it in the landscapeRaster
			if(weatherRaster.header.xllCorner - landscapeCoord[0] > landscapeRaster.header.cellsize/2.0) {
				//We're closer to the right cell than the left, so snap to that one
				weatherRaster.header.xllCorner = landscapeCoord[0] + landscapeRaster.header.cellsize;
			} else {
				//We're closer to the left cell than the right, so snap to that one
				weatherRaster.header.xllCorner = landscapeCoord[0];
			}

			if(weatherRaster.header.yllCorner - landscapeCoord[1] > landscapeRaster.header.cellsize/2.0) {
				//We're closer to the top cell than the bottom, so snap to that one
				weatherRaster.header.yllCorner = landscapeCoord[1] + landscapeRaster.header.cellsize;
			} else {
				//We're closer to the bottom cell than the top, so snap to that one
				weatherRaster.header.yllCorner = landscapeCoord[1];
			}

		} else {
			reportReadError("Unable to read weather file \"%s\"\n", weatherFileName.c_str());
		}

	}

	return 1;

}

void WeatherDatabase::addFileToQueue(string sFileName) {

	if(mapIDtoRasters.find(sFileName) == mapIDtoRasters.end() && !(sFileName.empty())) {
		mapIDtoRasters[sFileName] = Raster<double>();
	}

}

void WeatherDatabase::applyFileIDtoProperty(string sID, vector<PopModPair> &vTargets) {

	//cout << "Applying weather from file " << sID << endl;

	Raster<double> &weatherRaster = mapIDtoRasters[sID];

	//Fake LandscapeRaster for raster translation
	Raster<int> landscapeRaster(1,1, world->header->xllCorner, world->header->yllCorner, world->header->cellsize, 0);
	landscapeRaster.header.nCols = world->header->nCols;
	landscapeRaster.header.nRows = world->header->nRows;

	//TODO: Invert this procedure:
	//go over each cell in landscape, find which (if any) cell in weather contains its centroid
	//Probably more efficient and don't need to fiddle with raster alignment - weather cell containing landscape cell centroid must always be the most overlapping one

	//For each cell in weatherRaster, apply to all locations which have centroids covered:
	for(int iWX=0; iWX<weatherRaster.header.nCols; iWX++) {
		for(int iWY=0; iWY<weatherRaster.header.nRows; iWY++) {

			double dVal = weatherRaster.getValue(iWX, iWY);

			vector<int> landscapeCellMin = landscapeRaster.realToCell(weatherRaster.cellToReal(iWX, iWY));
			vector<int> landscapeCellMax = landscapeRaster.realToCell(weatherRaster.cellToReal(iWX+1, iWY+1));

			if(landscapeCellMin[0] < 0) {
				landscapeCellMin[0] = 0;
			}
			if(landscapeCellMax[0] > world->header->nCols) {
				landscapeCellMax[0] = world->header->nCols;
			}

			if(landscapeCellMin[1] < 0) {
				landscapeCellMin[1] = 0;
			}
			if(landscapeCellMax[1] > world->header->nRows) {
				landscapeCellMax[1] = world->header->nRows;
			}


			for(int iLX=landscapeCellMin[0]; iLX<landscapeCellMax[0]; iLX++) {
				for(int iLY=landscapeCellMin[1]; iLY<landscapeCellMax[1]; iLY++) {

					LocationMultiHost *pLoc = world->locationArray[iLX + world->header->nCols*iLY];

					if(pLoc != NULL) {

						//Apply to all targets:
						for(size_t iTarget=0; iTarget<vTargets.size(); iTarget++) {

							switch(vTargets[iTarget].wmMod) {
							case SUSCEPTIBILITY:
								pLoc->setSusBase(dVal, vTargets[iTarget].iPop);
								break;
							case INFECTIVITY:
								pLoc->setInfBase(dVal, vTargets[iTarget].iPop);
								break;
							}

						}

					}

				}
			}

		}
	}

}
