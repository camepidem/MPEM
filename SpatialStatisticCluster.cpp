//SpatialStatisticCluster.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "ColourRamp.h"
#include <queue>
#include "SpatialStatisticCluster.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


SpatialStatisticCluster::SpatialStatisticCluster()
{

	//Memory housekeeping:
	pStatFile			= NULL;

}


SpatialStatisticCluster::~SpatialStatisticCluster()
{

	if(pStatFile) {
		fclose(pStatFile);
		pStatFile = NULL;
	}

}

int SpatialStatisticCluster::reset() {

	if(enabled) {
		setupFile();
	}

	return enabled;
}

int SpatialStatisticCluster::setup(std::vector<TargetData*> data) {

	enabled = 0;

	//Read data from master:
	char szKey[N_MAXSTRLEN];
	char *szStatName = "SpatialStatisticCluster";

	sprintf(szKey,"%s%s",szStatName, "Enable");
	readValueToProperty(szKey, &enabled, -2, "[0,1]");


	if(enabled) {

		bOutputImages = 0;

		sprintf(szKey,"%s%s",szStatName, "OutputImages");
		readValueToProperty(szKey, &bOutputImages, -2, "[0,1]");

	}


	if(enabled && !bIsKeyMode()) {
		setupFile();
	}

	return enabled;
}

std::map<std::string, double> SpatialStatisticCluster::calculateStats(double tNow, TargetData data, const Raster<int>* simulationState) {

	std::map<std::string, double> stats;

	stats["Time"] = tNow;

	Raster<int> *fillRaster = new Raster<int>();

	fillRaster->init(simulationState->header);
	fillRaster->setAllRasterValues(-1);

	vector<int> clusterSizes;

	//WARNING: BY DESIGN rasterToFloodFill is modified, make sure that do not pass the actual simulation data to this function
	int nClustersFound = floodfillClusters(fillRaster, simulationState, &clusterSizes);

	//TODO: DO SOMETHING WITH OUTPUT...

	if (bOutputImages) {
		char sTempName[_N_MAX_STATIC_BUFF_LEN];
		sprintf(sTempName, "%sCLUSTERVIEW_%.6f.png", szPrefixOutput(), tNow);

		ColourRamp *crTempColourRamp = new ColourRamp("DD", -1.0, nClustersFound - 1);

		//TODO: Make this usable even without the Video being explicitly enabled...
		writeRasterToFile(sTempName, fillRaster, crTempColourRamp);

		//Clean up the temporary colour ramp
		delete crTempColourRamp;
	}

	//DEBUG::
	//Incredibly, prohibitively, expensive for many clusters on large landscapes
	int bDoManualClusterSizeCheck = 0;
	vector<int> manualClusterSizes;
	if (bDoManualClusterSizeCheck) {

		for (int iCluster = 0; iCluster<int(clusterSizes.size()); iCluster++) {

			int nManualCount = 0;

			for (int iX = 0; iX<fillRaster->header.nCols; iX++) {
				for (int iY = 0; iY<fillRaster->header.nRows; iY++) {
					if (fillRaster->getValue(iX, iY) == iCluster) {
						nManualCount++;
					}
				}
			}

			manualClusterSizes.push_back(nManualCount);

		}
	}
	//DEBUG::

	//Remove temporary raster:
	delete fillRaster;


	stats["N_Clusters"] = nClustersFound;

	//TODO: Don't output this any more, as would need to be able to put a string into stats, but this would require everything being a string, which while possible will require redoing everything
	std::stringstream allTheData;

	for (int iCluster = 0; iCluster<int(clusterSizes.size()); iCluster++) {
		allTheData << clusterSizes[iCluster] << " ";
		//DEBUG::
		if (bDoManualClusterSizeCheck) {
			int nDelta = clusterSizes[iCluster] - manualClusterSizes[iCluster];
			if (nDelta != 0) {
				allTheData << "ERROR" << nDelta << " ";
			}
		}
		//DEBUG::
	}


	return stats;
}

std::vector<std::string> SpatialStatisticCluster::statsHeader() {

	std::vector<std::string> header;

	header.push_back("Time");

	//TODO: Figure out what to calculate:
	header.push_back("N_Clusters");

	return header;
}

void SpatialStatisticCluster::setupFile() {

	if(enabled) {
		if(pStatFile) {
			fclose(pStatFile);
			pStatFile = NULL;
		}

		char szFileName[N_MAXFNAMELEN];
		sprintf(szFileName, "%sSpatialStatistics_Cluster.txt",szPrefixOutput());
		pStatFile = fopen(szFileName,"w");
		if(pStatFile) {
			;
		} else {
			reportReadError("ERROR: Unable to write to file %s\n", szFileName);
			return;
		}

		//Write header line:
		auto header = statsHeader();
		for (auto headerElement : header) {
			fprintf(pStatFile, "%s ", headerElement.c_str());
		}
		fprintf(pStatFile, "\n");
	}
	return;
}

int SpatialStatisticCluster::floodfillClusters(Raster<int> *rasterToFloodFill, const Raster<int> *stateRaster, vector<int> *pClusterSizes, const Raster<int> *mask) {

	int nClustersFoundSoFar = 0;
	

	if(enabled) {

		//DEBUG::
		int iLastPercent = 0;
		//DEBUG::

		for(int iX=0; iX<stateRaster->header.nCols; iX++) {
			for(int iY=0; iY<stateRaster->header.nRows; iY++) {
				if(stateRaster->getValue(iX, iY) > 0 && rasterToFloodFill->getValue(iX, iY) < 0) {
					//int nClusterSize = floodFillRecursive(iX, iY, nClustersFoundSoFar, rasterToFloodFill, stateRaster);
					int nClusterSize = floodFillQueueBased(iX, iY, nClustersFoundSoFar, rasterToFloodFill, stateRaster);
					pClusterSizes->push_back(nClusterSize);
					nClustersFoundSoFar++;
				}
			}
			//DEBUG::
			/*if(int(100.0*iX/stateRaster->header.nCols) > iLastPercent) {
				iLastPercent++;
				printf("Floodfill %d%% done\n", iLastPercent);
			}*/
			//DEBUG::
		}

	}

	return nClustersFoundSoFar;
}


int SpatialStatisticCluster::floodFillRecursive(int iX, int iY, int clusterNo, Raster<int> *fillingRaster, const Raster<int> *stateRaster) {
	
	int nFilled = 1;

	//If location out of bounds, already filled or not valid
	if(iX < 0 || iX >= fillingRaster->header.nCols || iY < 0 || iY >= fillingRaster->header.nRows || fillingRaster->getValue(iX, iY) >= 0 || stateRaster->getValue(iX, iY) == 0) {
		return 0;
	}
	
	//Fresh cell, mark and move on:
	fillingRaster->setValue(iX, iY, clusterNo);

	//Floodfill about nn neighbourhood:
	//floodFill(iX, iY, clusterNo, fillingRaster, stateRaster);
	nFilled += floodFillRecursive(iX + 1, iY, clusterNo, fillingRaster, stateRaster);
	nFilled += floodFillRecursive(iX - 1, iY, clusterNo, fillingRaster, stateRaster);
	nFilled += floodFillRecursive(iX , iY + 1, clusterNo, fillingRaster, stateRaster);
	nFilled += floodFillRecursive(iX , iY - 1, clusterNo, fillingRaster, stateRaster);

	return nFilled;

}

//TODO: Change to breadth first traversal
int SpatialStatisticCluster::floodFillQueueBased(int iSourceX, int iSourceY, int clusterNo, Raster<int> *fillingRaster, const Raster<int> *stateRaster) {

	int nFilled = 0;

	std::queue<int> xQueue;
	std::queue<int> yQueue;

	xQueue.push(iSourceX);
	yQueue.push(iSourceY);

	while(!xQueue.empty()) {

		int cX = xQueue.front();xQueue.pop();
		int cY = yQueue.front();yQueue.pop();

		//For all possible neighbourhood of (cX,cY)
		//TODO: Only need to check this as are not checking the seed location... but already know the seed location is valid to call this function at all... bleh
		int iX = cX;
		int iY = cY;

		//Check if valid place
		if(iX >= 0 && iX < fillingRaster->header.nCols && iY >= 0 && iY < fillingRaster->header.nRows && fillingRaster->getValue(iX, iY) < 0 && stateRaster->getValue(iX, iY) == 1) {
			
			//Fresh cell, mark:
			fillingRaster->setValue(iX, iY, clusterNo);
			nFilled++;

			//Add as possible centre for next time:
			xQueue.push(iX);
			yQueue.push(iY);

		}

		//TODO: Horrible copypasta, replace with loop once neighbourhood concept has been sorted out:
		iX = cX + 1;
		iY = cY;
		//Check if valid place
		if(iX >= 0 && iX < fillingRaster->header.nCols && iY >= 0 && iY < fillingRaster->header.nRows && fillingRaster->getValue(iX, iY) < 0 && stateRaster->getValue(iX, iY) == 1) {
			
			//Fresh cell, mark:
			fillingRaster->setValue(iX, iY, clusterNo);
			nFilled++;

			//Add as possible centre for next time:
			xQueue.push(iX);
			yQueue.push(iY);

		}

		iX = cX - 1;
		iY = cY;
		//Check if valid place
		if(iX >= 0 && iX < fillingRaster->header.nCols && iY >= 0 && iY < fillingRaster->header.nRows && fillingRaster->getValue(iX, iY) < 0 && stateRaster->getValue(iX, iY) == 1) {
			
			//Fresh cell, mark:
			fillingRaster->setValue(iX, iY, clusterNo);
			nFilled++;

			//Add as possible centre for next time:
			xQueue.push(iX);
			yQueue.push(iY);

		}

		iX = cX;
		iY = cY + 1;
		//Check if valid place
		if(iX >= 0 && iX < fillingRaster->header.nCols && iY >= 0 && iY < fillingRaster->header.nRows && fillingRaster->getValue(iX, iY) < 0 && stateRaster->getValue(iX, iY) == 1) {
			
			//Fresh cell, mark:
			fillingRaster->setValue(iX, iY, clusterNo);
			nFilled++;

			//Add as possible centre for next time:
			xQueue.push(iX);
			yQueue.push(iY);

		}

		iX = cX;
		iY = cY - 1;
		//Check if valid place
		if(iX >= 0 && iX < fillingRaster->header.nCols && iY >= 0 && iY < fillingRaster->header.nRows && fillingRaster->getValue(iX, iY) < 0 && stateRaster->getValue(iX, iY) == 1) {
			
			//Fresh cell, mark:
			fillingRaster->setValue(iX, iY, clusterNo);
			nFilled++;

			//Add as possible centre for next time:
			xQueue.push(iX);
			yQueue.push(iY);

		}

	}


	return nFilled;
}
//int SpatialStatisticCluster::floodFillQueueBased(int iSourceX, int iSourceY, int clusterNo, Raster<int> *fillingRaster, Raster<int> *stateRaster) {
//
//	int nFilled = 0;
//
//	std::queue<int> xQueue;
//	std::queue<int> yQueue;
//
//	xQueue.push(iSourceX);
//	yQueue.push(iSourceY);
//
//	while(!xQueue.empty()) {
//
//		int iX = xQueue.front();
//		int iY = yQueue.front();
//
//		if(iX < 0 || iX >= fillingRaster->header.nCols || iY < 0 || iY >= fillingRaster->header.nRows || fillingRaster->getValue(iX, iY) >= 0 || stateRaster->getValue(iX, iY) == 0) {
//			
//			//Point not valid, bin it and carry on to next one:
//			xQueue.pop();
//			yQueue.pop();
//			continue;
//		}
//
//		//Fresh cell, mark and move on:
//		fillingRaster->setValue(iX, iY, clusterNo);
//		nFilled++;
//
//		//Add all neighbours to the queue:
//		//R
//		xQueue.push(iX + 1);
//		yQueue.push(iY);
//		
//		//L
//		xQueue.push(iX - 1);
//		yQueue.push(iY);
//
//		//U
//		xQueue.push(iX);
//		yQueue.push(iY + 1);
//
//		//D
//		xQueue.push(iX);
//		yQueue.push(iY - 1);
//
//	}
//
//
//	return nFilled;
//}
