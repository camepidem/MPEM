//InterScriptedInfectionData.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "PointXYV.h"
#include "RasterHeader.h"
#include "InterScriptedInfectionData.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


InterScriptedInfectionData::InterScriptedInfectionData() {
	timeSpent = 0;
	InterName = "ScriptedInfectionData";

	setCurrentWorkingSubection(InterName, CM_APPLICATION);


	enabled = 0;
	frequency = 1e30;
	timeFirst = world->timeEnd + frequency;
	timeNext = timeFirst;
	readValueToProperty("ScriptedInfectionDataEnable",&enabled,-1, "[0,1]");


	if(enabled) {

		nTimeSnapshots = 1;
		readValueToProperty("ScriptedInfectionDataNSnapshots", &nTimeSnapshots, 2, ">0");

	}
	printToKeyFile("//This then reads in a bunch of files that look like \"P_ScriptedInfectionDataSnapshot_INDEX.txt\".\n");

	if(enabled && !world->bGiveKeys) {

		char szFileName[_N_MAX_STATIC_BUFF_LEN];

		dataSnapshots.resize(nTimeSnapshots);
		pnDataPoints.resize(nTimeSnapshots);
		pdTimeSnaps.resize(nTimeSnapshots);

		for(int iSnap=0; iSnap<nTimeSnapshots; iSnap++) {

			//Get the data for this snapshot:
			sprintf(szFileName, "P_ScriptedInfectionDataSnapshot_%d.txt", iSnap);
			if(!readIntFromCfg(szFileName, "Points", &pnDataPoints[iSnap]) || pnDataPoints[iSnap] <= 0) {
				reportReadError("ERROR: in file %s found number of points: %d <=0\n",szFileName, pnDataPoints[iSnap]);
			}

			if(!readDoubleFromCfg(szFileName, "Time", &pdTimeSnaps[iSnap])) {
				reportReadError("ERROR: Could not read Time from file %s\n",szFileName);
			}

			//Read from the points file:
			vector<int> tempX(pnDataPoints[iSnap]);
			vector<int> tempY(pnDataPoints[iSnap]);
			vector<int> tempZ(pnDataPoints[iSnap]);

			if(!readTable(szFileName, pnDataPoints[iSnap], 2, &tempX[0], &tempY[0], &tempZ[0])) {//Skip 2 lines of header here
				reportReadError("ERROR: unable to read from %s successfully\n", szFileName);
			}

			dataSnapshots[iSnap].resize(pnDataPoints[iSnap]);

			for(int iPoint=0; iPoint<pnDataPoints[iSnap]; iPoint++) {
				int x = tempX[iPoint];
				int y = tempY[iPoint];
				if(x >= 0 && x < world->header->nCols && y >= 0 && y < world->header->nRows) {
					dataSnapshots[iSnap][iPoint] = new PointXYV<int>(x, y, tempZ[iPoint]);
				} else {
					reportReadError("ERROR: Specified point %d in file %s at x=%d, y=%d is outside the bounds of the landscape: x in [0, %d] y in [0, %d]\n", iPoint, szFileName, x, y, world->header->nCols-1, world->header->nRows-1);
				}
			}

		}

		//Initialise to first snapshot:
		findNextSnapshot(1);

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterScriptedInfectionData::~InterScriptedInfectionData() {
	
	for (size_t iSnap = 0; iSnap < dataSnapshots.size(); iSnap++) {
		for (size_t iPoint = 0; iPoint < dataSnapshots[iSnap].size(); iPoint++) {
			delete dataSnapshots[iSnap][iPoint];
		}
	}

}

int InterScriptedInfectionData::intervene() {
	//printf("SCRIPTED INFECTION!\n");

	if(enabled) {

		//std::cerr << "ScriptedInfection" << std::endl;

		//Modify system state as specified in snapshot:
		int nPoints = pnDataPoints[iCurrentSnapshot];

		PointXYV<int> **currentSnapshot = &dataSnapshots[iCurrentSnapshot][0];
		//TODO: Generalised transitionHosts function move all|arbitrary classes to other classes
		int nPointsChanged = 0;
		for(int iPoint=0; iPoint<nPoints; iPoint++) {
			PointXYV<int> *currentPoint = currentSnapshot[iPoint];
			LocationMultiHost *tgtLoc = world->locationArray[currentPoint->x + world->header->nCols*currentPoint->y];
			if(tgtLoc != NULL) {
				switch(currentPoint->value) {
				case -1:
					//Make target site susceptible no matter what:
					if (tgtLoc->causeNonSymptomatic(-1) != 0) {
						nPointsChanged++;
					}
					break;
				case 0:
					//Make target site susceptible:
					if(tgtLoc->getDetectableCoverageProportion(-1) > 0.0) {
						//Site is symptomatic:
						if (tgtLoc->causeNonSymptomatic(-1) != 0) {
							nPointsChanged++;
						}
					}
					break;
				case 1:
					//Make target site symptomatic:
					if(tgtLoc->getDetectableCoverageProportion(-1) == 0.0) {
						//Site is not already symptomatic:
						if (tgtLoc->causeSymptomatic(-1) != 0) {
							nPointsChanged++;
						}
					}
					break;
				case 2:
					//Make target site dead:
					if (tgtLoc->causeRemoved(-1)) {
						nPointsChanged++;
					}
					break;
				case 3:
					//Make target site cryptic:
					if (tgtLoc->causeCryptic(-1)) {
						nPointsChanged++;
					}
				case 4:
					//Make target site exposed:
					if (tgtLoc->causeExposed(-1)) {
						nPointsChanged++;
					}
				case 5:
					//Make target site exposed:
					//Make only a single host exposed - could never possibly have more than 1e9 hosts in a cell...?
					if (tgtLoc->causeExposed(-1, 1e-9)) {
						nPointsChanged++;
					}
				default:
					break;
				}
			}
		}

		//If have actually made a change, will need to recalculate landscape rates:
		if(nPointsChanged >= 0) {
			//TODO: Might not need to do this all the time?
			//TODO: Can either get away with just an InfectionRateRecalculation or nothing at all?
			//std::cerr << "ScriptedInfection made " << nPointsChanged << " changes" << std::endl;
			//world->bForceDebugFullRecalculation = 1;
		}

		//printf("DEBUG: SCRIPTEDINFECTION CHANGED %d\n", nPointsChanged);

		findNextSnapshot();

	} else {
		timeNext = world->timeEnd + 1e30;
	}

	return 1;
}

int InterScriptedInfectionData::reset() {
	//printf("InterScriptedInfectionData::reset()\n");

	if(enabled) {
		findNextSnapshot(1);
	}

	return 1;
}

void InterScriptedInfectionData::findNextSnapshot(int bReset) {

	if(bReset) {
		//Find least time >= simulation start time:
		iCurrentSnapshot = -1;
		timeNext = world->timeStart + 1e30;

		int nAppliedSnapshots = 0;

		for(int iSnapshot=0; iSnapshot<nTimeSnapshots; ++iSnapshot) {
			if(pdTimeSnaps[iSnapshot] >= world->timeStart) {
				nAppliedSnapshots++;
				if (pdTimeSnaps[iSnapshot] < timeNext) {
					iCurrentSnapshot = iSnapshot;
					timeNext = pdTimeSnaps[iSnapshot];
				}
			}
		}

		if (iCurrentSnapshot == -1) {
			printk("Warning: All specified Scripted Infection data is scheduled to take place before the start of the simulation. None will be applied.\n");
		} else {
			if (nAppliedSnapshots < nTimeSnapshots) {
				printk("Warning: Scripted Infection data specifies %d snapshots, but only %d are after the start of the simulation. Only the ones at or after the start will be applied.\n", nTimeSnapshots, nAppliedSnapshots);
			}
		}

	} else {

		//Find the next time of a snapshot:

		//First (pathological case): sweep to the right for snapshots that have the same time as current one
		//Using the ordering of the array prevents getting stuck in an infinite loop with multiple simultaneous snapshots
		for(int iSnapshot=iCurrentSnapshot + 1; iSnapshot<nTimeSnapshots; ++iSnapshot) {
			if(pdTimeSnaps[iSnapshot] == timeNext) {
				iCurrentSnapshot = iSnapshot;
				timeNext = pdTimeSnaps[iSnapshot];
				return;
			}
		}

		//If there are no simultaneous snapshots to resolve:
		//Find the next snapshot after the current one
		double timePrevious = timeNext;

		int bFoundAnyViable = 0;

		for(int iSnapshot=0; iSnapshot<nTimeSnapshots; ++iSnapshot) {
			if(pdTimeSnaps[iSnapshot] > timePrevious) {
				//Viable:
				if(bFoundAnyViable) {
					//See if this is sooner than best found so far:
					if(pdTimeSnaps[iSnapshot] < timeNext) {
						//This one is sooner:
						iCurrentSnapshot = iSnapshot;
						timeNext = pdTimeSnaps[iSnapshot];
					}
				} else {
					//First viable one found, so go with this one for now:
					iCurrentSnapshot = iSnapshot;
					timeNext = pdTimeSnaps[iSnapshot];

					bFoundAnyViable = 1;
				}

			}
		}

		if(!bFoundAnyViable) {
			timeNext = world->timeEnd + 1e30;
			iCurrentSnapshot = -1;
		}

	}

}

int InterScriptedInfectionData::finalAction() {
	//printf("InterScriptedInfectionData::finalAction()\n");

	return 1;
}

void InterScriptedInfectionData::writeSummaryData(FILE *pFile) {

	//TODO: Output information on what was done where

	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Ok\n\n");
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
