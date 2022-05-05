//InterControlCull.cpp

#include "stdafx.h"
#include "cfgutils.h"
#include "json/json.h"
#include "Landscape.h"
#include "LocationMultiHost.h"
#include "PopulationCharacteristics.h"
#include "Population.h"
#include "ListManager.h"
#include "RasterHeader.h"
#include "Raster.h"
#include "myRand.h"
#include <algorithm>
#include "SummaryDataManager.h"//DEBUG
#include "InterControlCull.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */


//TODO: Manager for generalised control interventions allow things of form Control*IDENTIFIER*Enable 0/1
//Parse master file using string find for "CONTROL" ... "ENABLE" and init instances with InterName as Control*IDENTIFIER*
//Existing controls still exist but become special cases...

const char *InterControlCull::ControlTypeNames[] = {"NONE", "CULL", "ROGUE", "AS_SPRAY", "PR_SPRAY", "BOTH_SPRAY"};
const char *InterControlCull::ControlMethodSpecificationNames[] = {"BLANKET", "RASTER"};
const char *InterControlCull::ControlIntensitySpecificationNames[] = {"BLANKET", "RASTER"};
template <>
const InterControlCull::ControlType RasterHeader<InterControlCull::ControlType>::NODATA_default = InterControlCull::ControlType::CT_NONE;

InterControlCull::InterControlCull() {

	//Memory housekeeping:

	szCM_BlanketName		= NULL;
	szCM_RasterName			= NULL;
	prcmCM_Raster			= NULL;
	szCI_RasterName			= NULL;
	prCI_RasterRadius		= NULL;
	szPriorityRasterName	= NULL;
	prPriorities			= NULL;
	InterName				= NULL;


	timeSpent = 0;
	totalCost = 0.0;

	char *szControlID;
	InterName = new char[_N_MAX_DYNAMIC_BUFF_LEN];

	//TODO: Get the ID from manager...
	szControlID = "Cull";
	sprintf(InterName, "Control%s", szControlID);

	setCurrentWorkingSubection(InterName, CM_POLICY);

	//Read Data:
	char szKey[_N_MAX_STATIC_BUFF_LEN];

	enabled = 0;
	sprintf(szKey, "%sEnable", InterName);
	readValueToProperty(szKey, &enabled, -1, "[0,1]");

	if (enabled) {
		sprintf(szKey, "%sFrequency", InterName);
		frequency = 1.0;
		readValueToProperty(szKey, &frequency, 2, ">0");

		if (frequency <= 0.0) {
			reportReadError("ERROR: %s specified %s = %f <= 0.0, %s must be > 0.0\n", InterName, szKey, frequency, szKey);
		}

		sprintf(szKey, "%sFirst", InterName);
		timeFirst = world->timeStart + frequency;
		readValueToProperty(szKey, &timeFirst, 2, "");
		timeNextDynamicControl = timeFirst;

		//It used to be that a timeLast specified before the start of the simulation meant to control without a time limit, 
		//however with configurable start time this could lead to ambiguity: 
		//a start time being moved later would first decrease the amount of control applied, then once the last control was before the start time,
		//control would be performed forever, however the desired behaviour would be to not control at all...
		//Solution: have timeLast be absurdly huge if want no time limit, world->timeEnd + 1.0(e3?) and if last is before start no control happens
		timeLast = world->timeEnd + 1.0e3 * frequency;
		sprintf(szKey, "%sLast", InterName);
		readValueToProperty(szKey, &timeLast, -2, "");

		if (timeLast < timeFirst) {
			reportReadError("ERROR: %sLast was specified at time %f, which is prior to %sFirst = %f. Last time must be at or after first time.\n", szControlID, timeLast, szControlID, timeFirst);
		}

		if (timeFirst < world->timeStart) {
			printk("Warning: %sFirst (%f) was specified for before the simulation start time (%f). No controls will be applied before the start of the simulation.\n", szControlID, timeFirst, world->timeStart);

			//Advance to first time greater than or equal to simulation start:
			int nPeriodsBehind = int(ceil((world->timeStart - timeFirst) / frequency));
			double newTimeFirst = timeFirst + nPeriodsBehind * frequency;

			printk("Warning: Advancing %s first time to earliest time >= simulation start time by advancing start time %d periods to %f.\n", szControlID, nPeriodsBehind, newTimeFirst);

			timeFirst = newTimeFirst;

			if (timeFirst > timeLast) {
				printk("Warning: %s timeFirst (%f) is now greater than timeLast (%f), so no controls will be applied.\n", szControlID, timeFirst, timeLast);
			}
		}

		//TODO: method specification:
		cmsControlMethod = CMS_BLANKET;
		//Read what control method to do
		char szCMS_Name[_N_MAX_STATIC_BUFF_LEN];
		sprintf(szCMS_Name, "%s", ControlMethodSpecificationNames[cmsControlMethod]);
		sprintf(szKey, "%sControlMethodSpecification", InterName);
		char sMethodSpecificationList[_N_MAX_STATIC_BUFF_LEN];
		sprintf(sMethodSpecificationList, "{");
		for(int iCMS=0; iCMS<CMS_MAX; iCMS++) {
			if(iCMS>0) {
				strcat(sMethodSpecificationList, ",");
			}
			strcat(sMethodSpecificationList, ControlMethodSpecificationNames[iCMS]);
		}
		strcat(sMethodSpecificationList, "}");
		readValueToProperty(szKey, szCMS_Name, -2, sMethodSpecificationList);

		cmsControlMethod = cmsNameToCMS(szCMS_Name);


		szCM_BlanketName = new char[_N_MAX_DYNAMIC_BUFF_LEN];
		if(cmsControlMethod == CMS_BLANKET || bIsKeyMode()) {

			ctControlMethodBlanketType = CT_CULL;
			//Read what control to do
			sprintf(szCM_BlanketName, "%s", ControlTypeNames[CT_CULL]);
			sprintf(szKey, "%sControlMethodBlanketName", InterName);
			char sTypeList[_N_MAX_STATIC_BUFF_LEN];
			sprintf(sTypeList, "{");
			for(int iCTN=0; iCTN<CT_MAX; iCTN++) {
				if(iCTN>0) {
					strcat(sTypeList, ",");
				}
				strcat(sTypeList, ControlTypeNames[iCTN]);
			}
			strcat(sTypeList, "}");
			readValueToProperty(szKey, szCM_BlanketName, -3, sTypeList);

		}

		szCM_RasterName = new char[_N_MAX_DYNAMIC_BUFF_LEN];
		if(cmsControlMethod == CMS_RASTER || bIsKeyMode()) {

			sprintf(szKey, "%sControlMethodRasterName", InterName);
			sprintf(szCM_RasterName, "%s%s%s", world->szPrefixLandscapeRaster, "ControlMethodRaster", world->szSuffixTextFile);
			readValueToProperty(szKey, szCM_RasterName, -3, "#FileName#");

		}

		//Intensity specification:
		cisControlIntensity = CIS_BLANKET;
		//Read what control intensity to do
		char szCIS_Name[_N_MAX_STATIC_BUFF_LEN];
		sprintf(szCIS_Name, "%s", ControlIntensitySpecificationNames[cisControlIntensity]);
		sprintf(szKey, "%sControlIntensitySpecification", InterName);
		char sIntensitySpecificationList[_N_MAX_STATIC_BUFF_LEN];
		sprintf(sIntensitySpecificationList, "{");
		for(int iCIS=0; iCIS<CIS_MAX; iCIS++) {
			if(iCIS>0) {
				strcat(sIntensitySpecificationList, ",");
			}
			strcat(sIntensitySpecificationList, ControlIntensitySpecificationNames[iCIS]);
		}
		strcat(sIntensitySpecificationList, "}");
		readValueToProperty(szKey, szCIS_Name, -2, sIntensitySpecificationList);

		cisControlIntensity = cisNameToCIS(szCIS_Name);


		if(cisControlIntensity == CIS_BLANKET || bIsKeyMode()) {

			dCI_BlanketRadius = 1.0;
			sprintf(szKey, "%sRadius", InterName);
			readValueToProperty(szKey, &dCI_BlanketRadius, -3, ">=-1");

		}

		szCI_RasterName = new char[_N_MAX_DYNAMIC_BUFF_LEN];
		if(cisControlIntensity == CIS_FROMRASTER || bIsKeyMode()) {

			sprintf(szKey, "%sRadiusRasterName", InterName);
			sprintf(szCI_RasterName, "%s%s%s", world->szPrefixLandscapeRaster, "ControlRadiusRaster", world->szSuffixTextFile);
			readValueToProperty(szKey, szCI_RasterName, -3, "#FileName#");

		}

		effectiveness = 1.0;
		sprintf(szKey, "%sEffectiveness", InterName);
		readValueToProperty(szKey, &effectiveness, -2, "[0,1]");

		if(effectiveness > 1.0 || effectiveness < 0.0) {
			reportReadError("ERROR: %sEffectiveness must be in range 0.0 to 1.0, read: %f\n",InterName, effectiveness);
		}

		forgets = 0;
		sprintf(szKey, "%sForget", InterName);
		readValueToProperty(szKey, &forgets, -2, "[0,1]");


		sprayPRProps.decays = 0;
		sprintf(szKey, "%sSprayPRDecays", InterName);
		readValueToProperty(szKey, &sprayPRProps.decays, -2, ">=0");

		if (sprayPRProps.decays) {

			sprayPRProps.decayFactor = 0.0;
			sprintf(szKey, "%sSprayPRDecayFactor", InterName);
			readValueToProperty(szKey, &sprayPRProps.decayFactor, -3, "[0,1)");

			if (sprayPRProps.decayFactor < 0.0 || sprayPRProps.decayFactor >= 1.0 && !world->bGiveKeys) {
				reportReadError("ERROR: %sSprayPRDecayFactor must be in the interval [0,1). Read %.6f\n", InterName, sprayPRProps.decayFactor);
				if (sprayPRProps.decayFactor == 1.0) {
					reportReadError("A value of 1.0 is not allowed, do not enable decay if you do not want decay.\n");
				}
			}

			sprayPRProps.decayPeriod = 1.0;
			sprintf(szKey, "%sSprayPRDecayPeriod", InterName);
			readValueToProperty(szKey, &sprayPRProps.decayPeriod, -3, ">0.0");

			if (sprayPRProps.decayPeriod <= 0.0) {
				reportReadError("ERROR: %sSprayPRDecayFactor must be > 0.0. Read %.6f\n", InterName, sprayPRProps.decayPeriod);
			}

			sprayPRProps.minEffectiveness = 0.01;
			sprintf(szKey, "%sSprayPRMinEffectiveness", InterName);
			readValueToProperty(szKey, &sprayPRProps.minEffectiveness, -3, ">=0.0");

			if (sprayPRProps.minEffectiveness < 0.0) {
				reportReadError("ERROR: %sSprayPRDecayFactor must be >= 0.0. Read %.6f\n", InterName, sprayPRProps.minEffectiveness);
			}
		}

		sprayASProps.decays = 0;
		sprintf(szKey, "%sSprayASDecays", InterName);
		readValueToProperty(szKey, &sprayASProps.decays, -2, ">=0");

		if (sprayASProps.decays) {

			sprayASProps.decayFactor = 0.0;
			sprintf(szKey, "%sSprayASDecayFactor", InterName);
			readValueToProperty(szKey, &sprayASProps.decayFactor, -3, "[0,1)");

			if (sprayASProps.decayFactor < 0.0 || sprayASProps.decayFactor >= 1.0 && !world->bGiveKeys) {
				reportReadError("ERROR: %sSprayASDecayFactor must be in the interval [0,1). Read %.6f\n", InterName, sprayASProps.decayFactor);
				if (sprayASProps.decayFactor == 1.0) {
					reportReadError("A value of 1.0 is not allowed, do not enable decay if you do not want decay.\n");
				}
			}

			sprayASProps.decayPeriod = 1.0;
			sprintf(szKey, "%sSprayASDecayPeriod", InterName);
			readValueToProperty(szKey, &sprayASProps.decayPeriod, -3, ">0.0");

			if (sprayASProps.decayPeriod <= 0.0) {
				reportReadError("ERROR: %sSprayASDecayFactor must be > 0.0. Read %.6f\n", InterName, sprayASProps.decayPeriod);
			}

			sprayASProps.minEffectiveness = 0.01;
			sprintf(szKey, "%sSprayASMinEffectiveness", InterName);
			readValueToProperty(szKey, &sprayASProps.minEffectiveness, -3, ">=0.0");

			if (sprayASProps.minEffectiveness < 0.0) {
				reportReadError("ERROR: %sSprayASDecayFactor must be >= 0.0. Read %.6f\n", InterName, sprayASProps.minEffectiveness);
			}
		}

		//TODO: Option to control on a per population basis?
		iPopulationToControl = PopulationCharacteristics::ALL_POPULATIONS;

		bPriorityBasedControl = 0;
		sprintf(szKey, "%sPriorityBasedControl", InterName);
		readValueToProperty(szKey, &bPriorityBasedControl, -2, "[0,1]");

		dTotalBudgetPerSweep = 1.0;
		sprintf(szKey, "%sTotalBudgetPerSweep", InterName);
		readValueToProperty(szKey, &dTotalBudgetPerSweep, -2, ">0");

		dCostPerVisit = 0.0;
		sprintf(szKey, "%sCostPerVisit", InterName);
		readValueToProperty(szKey, &dCostPerVisit, -2, ">=0");

		dCostPerUnitHost = 1.0;
		sprintf(szKey, "%sCostPerUnitHost", InterName);
		readValueToProperty(szKey, &dCostPerUnitHost, -2, ">=0");

		//TODO: Additional priority systems:
		iPrioritySystem = PRI_RANDOM;
		char szPrioritySystemName[_N_MAX_STATIC_BUFF_LEN];
		sprintf(szPrioritySystemName, "Random");
		sprintf(szKey, "%sPrioritySystem", InterName);
		readValueToProperty(szKey, szPrioritySystemName, -3, "{Random, Raster}");

		szPriorityRasterName = new char[_N_MAX_DYNAMIC_BUFF_LEN];
		sprintf(szPriorityRasterName, "%sControlPriorityRaster%s", world->szPrefixLandscapeRaster, world->szSuffixTextFile);
		sprintf(szKey, "%sPriorityRasterName", InterName);
		readValueToProperty(szKey, szPriorityRasterName, -4, "#FileName#");

		//Scripted controls:
		bUseScriptedControls = 0;
		sprintf(szKey, "%sScriptedControlEnable", InterName);
		if (readValueToProperty(szKey, &bUseScriptedControls, -2, "[0,1]")) {
			scriptedControlFileName = "ScriptedControls.json";
			char szTempScriptedControlFileName[_N_MAX_STATIC_BUFF_LEN];
			sprintf(szTempScriptedControlFileName, "%s", scriptedControlFileName.c_str());
			sprintf(szKey, "%sScriptedControlFileName", InterName);
			if (readValueToProperty(szKey, szTempScriptedControlFileName, -3, "#FileName#")) {
				scriptedControlFileName = szTempScriptedControlFileName;
			}
		}


		if(enabled && !world->bGiveKeys) {

			//Set up intervention:
			if(cmsControlMethod == CMS_BLANKET) {

				makeUpper(szCM_BlanketName);
				ctControlMethodBlanketType = ctNameToCT(szCM_BlanketName);

				if(ctControlMethodBlanketType == CT_MAX) {
					reportReadError("ERROR: Unable to parse control type from: %s\n",szCM_BlanketName);
					reportReadError("Valid control types are:\n");
					for(int iCT=0; iCT<CT_MAX; iCT++) {
						reportReadError("%s\n", ControlTypeNames[iCT]);
					}
				}

				if(ctControlMethodBlanketType == CT_CULL || ctControlMethodBlanketType == CT_ROGUE) {
					//Check it is actually possible to cull or rogue all target populations (ie they have a removed class)
					for(int iPopulation=0; iPopulation<PopulationCharacteristics::nPopulations; iPopulation++) {
						if(Population::pGlobalPopStats[iPopulation]->iRemovedClass < 0) {
							reportReadError("ERROR: Population %d cannot be culled or rogued because it does not have a removed class. ", iPopulation);
							reportReadError("Model: %s\n", Population::pGlobalPopStats[iPopulation]->szModelString);
						}
					}
				}

			}

			//TODO: Read method raster:
			if (cmsControlMethod == CMS_RASTER) {

				//TODO: Read raster of words or ids to temp raster, then write to the actual raster...

				Raster<std::string> tempCTRaster;

				if (!tempCTRaster.init(std::string(szCM_RasterName))) {
					reportReadError("ERROR: %s Unable to read ControlMethod raster: %s\n", InterName, szPriorityRasterName);
				}
				
				prcmCM_Raster = new Raster<ControlType>(tempCTRaster.header.nRows, tempCTRaster.header.nCols, tempCTRaster.header.xllCorner, tempCTRaster.header.yllCorner, tempCTRaster.header.cellsize, false);

				for (int iRow = 0; iRow < tempCTRaster.header.nRows; ++iRow) {
					for (int iCol = 0; iCol < tempCTRaster.header.nCols; ++iCol) {

						const char *ctNameString = tempCTRaster(iCol, iRow).c_str();

						ControlType locCT = ctNameToCT(ctNameString);

						if (locCT == CT_MAX) {
							reportReadError("ERROR: At row=%d, col=%d in file %s: Unable to parse \"%s\" to control method name.", iRow, iCol, ctNameString);
						} else {
							(*prcmCM_Raster)(iCol, iRow) = locCT;
						}

					}
				}

			}


			//TODO: formalise the selection of mode...
			makeUpper(szPrioritySystemName);

			int bFoundMatch = 0;

			if(strcmp(szPrioritySystemName, "RANDOM") == 0) {
				iPrioritySystem = PRI_RANDOM;
				bFoundMatch = 1;
			}
			if(strcmp(szPrioritySystemName, "RASTER") == 0) {
				iPrioritySystem = PRI_RASTER;
				bFoundMatch = 1;
			}

			if(!bFoundMatch) {
				reportReadError("ERROR: Unable to get priority system from specified: %s\n", szPrioritySystemName);
			}

			//Read intensity raster:
			if(cisControlIntensity == CIS_FROMRASTER) {
				prCI_RasterRadius = new Raster<double>();
				if(!prCI_RasterRadius->init(std::string(szCI_RasterName))) {
					reportReadError("ERROR: %s Unable to read ControlIntensity raster: %s\n", InterName, szPriorityRasterName);
				}
			}

			//Get the priority raster if necessary:
			if(bPriorityBasedControl && iPrioritySystem == PRI_RASTER) {
				prPriorities = new Raster<double>();
				if(!prPriorities->init(std::string(szPriorityRasterName))) {
					reportReadError("ERROR: %s Unable to read priority raster: %s\n", InterName, szPriorityRasterName);
				}
			}

			//Spray decay:
			activeSprayAS = std::map<LocationMultiHost*, SprayEffect>();
			activeSprayPR = std::map<LocationMultiHost*, SprayEffect>();
			timeNextSprayDecay = world->timeEnd + 1e30;

			//Scripted controls:
			if (bUseScriptedControls) {
				Json::Value root;

				std::ifstream cfgFile(scriptedControlFileName);
				cfgFile >> root;

				for(const auto &controlData : root) {
					ScriptedControlData scriptedControl;

					if (controlData.isMember("time")) {
						if (controlData["time"].isDouble()) {
							scriptedControl.applicationTime = controlData["time"].asDouble();
						} else {
							reportReadError("ERROR: Unable to parse \"time\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
						}
					} else {
						reportReadError("ERROR: Missing required element \"time\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
					}

					scriptedControl.locations;
					if (controlData.isMember("locations")) {
						if (controlData["locations"].isArray()) {
							for (const auto &loc : controlData["locations"]) {

								int locX, locY;
								bool ok = true;

								if (loc.isMember("x")) {
									if (loc["x"].isInt()) {
										locX = loc["x"].asInt();
									} else {
										reportReadError("ERROR: x must be an integer in Scripted Control location data: %s\n", loc.toStyledString().c_str()); ok = false;
									}
								} else {
									reportReadError("ERROR: Missing required element \"x\" from Scripted Control location data: %s\n", loc.toStyledString().c_str()); ok = false;
								}
								
								if (loc.isMember("y")) {
									if (loc["y"].isInt()) {
										locY = loc["y"].asInt();
									} else {
										reportReadError("ERROR: y must be an integer in Scripted Control location data: %s\n", loc.toStyledString().c_str()); ok = false;
									}
								} else {
									reportReadError("ERROR: Missing required element \"y\" from Scripted Control location data: %s\n", loc.toStyledString().c_str()); ok = false;
								}

								if (ok) {
									if (locX < 0 || locX >= world->header->nCols) {
										reportReadError("ERROR: x must be a valid cell coordinate in the range [0,%d] in Scripted Control location data: %s\n", world->header->nCols - 1, loc.toStyledString().c_str()); ok = false;
									}
									if (locY < 0 || locY >= world->header->nRows) {
										reportReadError("ERROR: y must be a valid cell coordinate in the range [0,%d] in Scripted Control location data: %s\n", world->header->nRows - 1, loc.toStyledString().c_str()); ok = false;
									}

									if (ok) {
										LocationMultiHost *targetLocation = world->locationArray[locX + world->header->nCols * locY];
										if (targetLocation != NULL) {
											//Totally empty locations aren't a problem, but we don't want to waste time controlling them
											scriptedControl.locations.push_back(targetLocation);
										}
									}
								}

							}
						}
					} else {
						reportReadError("ERROR: Missing required element \"locations\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
					}

					scriptedControl.type = ControlType::CT_CULL;
					if (controlData.isMember("type")) {
						if (controlData["type"].isString()) {

							auto typeString = controlData["type"].asString();
							
							scriptedControl.type = ctNameToCT(typeString.c_str());

							if (scriptedControl.type == ControlType::CT_MAX) {
								reportReadError("ERROR: Unable to parse \"type\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
								reportReadError("Valid types are:\n");
								for (int iCT = 0; iCT<CT_MAX; iCT++) {
									reportReadError("%s\n", ControlTypeNames[iCT]);
								}
							}
						} else {
							reportReadError("ERROR: Unable to parse \"type\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
						}
					}

					scriptedControl.targetPopulation = PopulationCharacteristics::ALL_POPULATIONS;
					if (controlData.isMember("population")) {
						if (controlData["population"].isInt()) {
							scriptedControl.targetPopulation = controlData["population"].asInt();
							if ((scriptedControl.targetPopulation >= PopulationCharacteristics::nPopulations || scriptedControl.targetPopulation < 0) && scriptedControl.targetPopulation != PopulationCharacteristics::ALL_POPULATIONS) {
								reportReadError("ERROR: Invalid value for \"population\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
								reportReadError("Valid values for population are:\n %d : All populations, or an integer in the range [0, %d] indicating a specific population\n", PopulationCharacteristics::ALL_POPULATIONS, PopulationCharacteristics::nPopulations - 1);
							}

							//Spray decay information is currently stored on a per-location basis, so we can't allow users to specify spraying for some populations and not others
							//This will require users, when using spraying, to specify either all populations, or in the case where there is only one population, that population
							if ((scriptedControl.type == CT_SPRAYAS || scriptedControl.type == CT_SPRAYPR || scriptedControl.type == CT_SPRAYBOTH) && (scriptedControl.targetPopulation != PopulationCharacteristics::ALL_POPULATIONS && PopulationCharacteristics::nPopulations > 1)) {
								reportReadError("ERROR: per-population scripted controls cannot apply sprays to individual populations, only all populations.\n");
							}

						} else {
							reportReadError("ERROR: Unable to parse \"population\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
						}
					}

					scriptedControl.effectiveness = 1.0;
					if (controlData.isMember("effectiveness")) {
						if (controlData["effectiveness"].isDouble()) {
							scriptedControl.effectiveness = controlData["effectiveness"].asDouble();
							if (scriptedControl.effectiveness < 0.0 || scriptedControl.effectiveness > 1.0) {
								reportReadError("ERROR: Invalid value for \"effectiveness\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
								reportReadError("Valid values for effectiveness are in the range [0.0, 1.0]\n");
							}
						} else {
							reportReadError("ERROR: Unable to parse \"effectiveness\" from Scripted Control configuration data: %s\n", controlData.toStyledString().c_str());
						}
					}


					scriptedControls.push_back(scriptedControl);
				}
			}

			timeDoneScriptedControl = world->timeStart - 1.0;
			timeNextScriptedControl = world->timeEnd + 1e30;
			//Find min time control time in object
			for (const auto &scriptedControl : scriptedControls) {
				timeNextScriptedControl = min(timeNextScriptedControl, scriptedControl.applicationTime);
			}
		}

		timeNext = min(timeNextDynamicControl, timeNextScriptedControl);

	} else {
		frequency = 1e30;
		timeFirst = world->timeEnd + 1e30;
		timeNext = timeFirst;
	}

}

InterControlCull::~InterControlCull() {

	if(szCM_BlanketName != NULL) {
		delete [] szCM_BlanketName;
		szCM_BlanketName = NULL;
	}

	if(szCM_RasterName != NULL) {
		delete [] szCM_RasterName;
		szCM_RasterName = NULL;
	}

	if(szCI_RasterName != NULL) {
		delete [] szCI_RasterName;
		szCI_RasterName = NULL;
	}

	if(szPriorityRasterName != NULL) {
		delete [] szPriorityRasterName;
		szPriorityRasterName = NULL;
	}

	if(InterName != NULL) {
		delete [] InterName;
		InterName = NULL;
	}

	if(prcmCM_Raster != NULL) {
		delete prcmCM_Raster;
		prcmCM_Raster = NULL;
	}

	if(prCI_RasterRadius != NULL) {
		delete prCI_RasterRadius;
		prCI_RasterRadius = NULL;
	}

	if(prPriorities != NULL) {
		delete prPriorities;
		prPriorities = NULL;
	}

}

bool priorityBasedSortingFunction(struct sLocPriorityPair a, struct sLocPriorityPair b) {
	return a.dPriority > b.dPriority;
}

int InterControlCull::intervene() {
	//printf("CULLING!\n");

	//Determine what we're doing:

	if (world->time >= timeNextDynamicControl) {

		if (world->time <= timeLast) {

			//Culling code
			//Go through each place in Landscape, add to list of places to control about:
			int nToControl = 0;
			LocationMultiHost *activeLocation;
			ListNode<LocationMultiHost> *pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;

			//TODO: Could do control by flagging all places to remove, then cluster analysis and then remove by cluster w/ piority per cluster...

			//Pass over all active locations
			while (pNode != NULL) {
				activeLocation = pNode->data;
				if (dShouldControlLocation(activeLocation) > 0.0) {
					nToControl++;
				}
				pNode = pNode->pNext;
			}

			char szControlFileName[256];

			sprintf(szControlFileName, "%sP_CONTROLCULL_%.6f%s", world->szPrefixOutput, world->time, world->szSuffixTextFile);

			FILE *pControlFile = fopen(szControlFileName, "w");

			fprintf(pControlFile, "X Y AMOUNT_CONTROLLED\n");

			if (nToControl > 0) {

				psLocationPriorityOrder.resize(nToControl);

				//Budget:
				double dRemainingBudget;
				if (bPriorityBasedControl) {
					dRemainingBudget = dTotalBudgetPerSweep;
				} else {
					dRemainingBudget = 2.0*(dCostPerVisit + dCostPerUnitHost*ListManager::locationLists[ListManager::iActiveList]->nLength)*nToControl;//More than it could possibly cost to do N controls each removing whole landscape...
				}

				pNode = ListManager::locationLists[ListManager::iActiveList]->pFirst->pNext;

				int iLocation = 0;
				//Pass over all active locations
				while (pNode != NULL) {
					activeLocation = pNode->data;
					double dPriority;
					if ((dPriority = dShouldControlLocation(activeLocation)) > 0.0) {
						psLocationPriorityOrder[iLocation].dPriority = dPriority;
						psLocationPriorityOrder[iLocation].pLoc = activeLocation;
						iLocation++;
					}
					pNode = pNode->pNext;
				}

				//Sort locations by priority:
				std::sort(psLocationPriorityOrder.begin(), psLocationPriorityOrder.end(), priorityBasedSortingFunction);

				//Cope with having many locations with same priority - don't want to always go left then down...
				//Shuffle contiguous blocks of the same value (post sorting so easy)
				int iBlockStart = 0;
				int iBlockEnd;
				while (iBlockStart < nToControl) {
					//Find contiguous block:
					double dBlockPriority = psLocationPriorityOrder[iBlockStart].dPriority;
					iBlockEnd = iBlockStart;
					while (iBlockEnd < nToControl && psLocationPriorityOrder[iBlockEnd].dPriority == dBlockPriority) {
						iBlockEnd++;
					}

					//Shuffle contiguous block: n.b. iBlockEnd is not part of the block
					for (int iBlockElement = iBlockStart; iBlockElement < iBlockEnd - 1; iBlockElement++) {  //last element need not swap 
						int iExchange = nUniformRandom(iBlockElement, iBlockEnd - 1);        //don't touch the elements that already "settled"

						//swap the items
						sLocPriorityPair tmp = psLocationPriorityOrder[iExchange];
						psLocationPriorityOrder[iExchange] = psLocationPriorityOrder[iBlockElement];
						psLocationPriorityOrder[iBlockElement] = tmp;
					}

					//Move on:
					iBlockStart = iBlockEnd;
				}

				//NOTE: This works better than expected wrt site visit costs as with radius only go there once but get a lot done...
				int nControlled = 0;

				//DEBUG
				//int iRemovedClass = 4;
				//double dRemovedAtStart = SummaryDataManager::getHostQuantities(0)[iRemovedClass];
				//double dCostAtStart = totalCost;

				while (nControlled < nToControl && dRemainingBudget > dCostPerVisit) {

					LocationMultiHost *LocationControlCentre = psLocationPriorityOrder[nControlled].pLoc;

					//DEBUG:
					/*FILE *pFile = fopen("DEBUGCULLFILE.txt", "a");
					fprintf(pFile, "%d\n", LocationControlCentre->xPos);
					fclose(pFile);*/

					double radius = dGetControlRadius(LocationControlCentre);

					//for all locations within radius about LocationControlCentre
					double r2 = (radius + 1.0)*(radius + 1.0);

					int centreX = LocationControlCentre->xPos;
					int centreY = LocationControlCentre->yPos;

					int startX = int(centreX - radius - 1.0);
					int startY = int(centreY - radius - 1.0);

					int endX = int(centreX + radius + 1.0);
					int endY = int(centreY + radius + 1.0);

					startX = max(0, startX);
					startY = max(0, startY);

					endX = min(world->header->nCols, endX);
					endY = min(world->header->nRows, endY);

					double dSweepCost = dCostPerVisit;

					for (int controlX = startX; controlX < endX; controlX++) {//TODO: Should do the control from the inside out in the case that budget might run out, as here we're chopping from the left of the circle
						for (int controlY = startY; controlY < endY; controlY++) {
							int u = (controlX - centreX);
							u *= u;
							int v = (controlY - centreY);
							v *= v;
							if (double(u + v) <= r2) {
								double dist = sqrt(double(u + v));

								//Avoid going over budget mid sweep:
								double maxControlExpenditure = dRemainingBudget - dSweepCost;
								double maxControlArea = 1.0;
								if (dCostPerUnitHost > 0.0) {
									maxControlArea = maxControlExpenditure / dCostPerUnitHost;
								}
								if (maxControlArea > 1.0) {
									maxControlArea = 1.0;
								}
								if (maxControlArea < 0.0) {
									maxControlArea = 0.0;
								}

								//Effectively control by approximate proportion of cell that is within radius
								double actualEffectiveness = 1.0;
								double areaProportionCoveredByControl = 1.0 + radius - dist;
								areaProportionCoveredByControl = min(max(actualEffectiveness, 0.0), 1.0);

								actualEffectiveness *= areaProportionCoveredByControl;

								actualEffectiveness *= effectiveness;

								if (actualEffectiveness > 0.0) {
									LocationMultiHost *tgtLoc = world->locationArray[controlX + world->header->nCols*controlY];
									if (tgtLoc != NULL) {

										double amountControlled = dControlLocation(tgtLoc, LocationControlCentre, ctGetControlType(LocationControlCentre), iPopulationToControl, actualEffectiveness, maxControlArea);

										dSweepCost += dCostPerUnitHost*amountControlled;

										if (amountControlled > 0.0) {
											fprintf(pControlFile, "%d %d %f\n", tgtLoc->xPos, tgtLoc->yPos, amountControlled);
										}
									}
								}
							}
						}
					}

					dRemainingBudget -= dSweepCost;
					totalCost += dSweepCost;

					if (forgets) {
						LocationControlCentre->setKnown(false, PopulationCharacteristics::ALL_POPULATIONS);
					}

					nControlled++;

				}

				//DEBUG:
				//double dRemovedAtEnd = SummaryDataManager::getHostQuantities(0)[iRemovedClass];
				//SummaryDataManager::pdHostQuantities[0][0][iRemovedClass]
				//double dCostAtEnd = totalCost;

				//printf("DEBUG::: REMOVED: BEFORE %f AFTER %f :::\n", dRemovedAtStart, dRemovedAtEnd);
				//printf("DEBUG::: COST:    BEFORE %f AFTER %f :::\n", dCostAtStart, dCostAtEnd);
				//printf("DEBUG::: CHANGE IN REMOVED: %f :::\n", (dRemovedAtEnd - dRemovedAtStart));
				//printf("DEBUG::: CHANGE IN COST:    %f :::\n", (dCostAtEnd - dCostAtStart));

				//because the cull has changed the landscape will need to 
				//reset last event information to force update of whole landscape after intervention
				world->bFlagNeedsFullRecalculation();
				//TODO: What about using the queue of places to update infectiousness from here?
			}

			fclose(pControlFile);

			timeNextDynamicControl += frequency;

			if (timeNextDynamicControl > timeLast) {
				//This was the last control:
				timeNextDynamicControl = world->timeEnd + 1e30;
			}

		} else {
			//Tried to control after last specified time:
			timeNextDynamicControl = world->timeEnd + 1e30;
		}

	}

	if (world->time >= timeNextScriptedControl) {
		applyScriptedControls();
	}

	if (world->time >= timeNextSprayDecay) {
		updateSprayStatus();
	}

	timeNext = min(timeNextDynamicControl, timeNextSprayDecay);
	timeNext = min(timeNext, timeNextScriptedControl);

	return 1;
}

int InterControlCull::reset() {

	if (enabled) {
		//Dynamic controls:
		timeNextDynamicControl = timeFirst;

		//Spray decay:
		//Remove any existing sprays
		activeSprayAS = std::map<LocationMultiHost*, SprayEffect>();
		activeSprayPR = std::map<LocationMultiHost*, SprayEffect>();
		timeNextSprayDecay = world->timeEnd + 1e30;

		//Scripted controls:
		timeDoneScriptedControl = world->timeStart - 1.0;
		timeNextScriptedControl = world->timeEnd + 1e30;
		//Find min time control time in object
		for (const auto &scriptedControl : scriptedControls) {
			timeNextScriptedControl = min(timeNextScriptedControl, scriptedControl.applicationTime);
		}

		//Find first overall event:
		timeNext = min(timeNextDynamicControl, timeNextSprayDecay);
		timeNext = min(timeNext, timeNextScriptedControl);


		totalCost = 0.0;
	}

	return enabled;
}

double InterControlCull::dShouldControlLocation(LocationMultiHost *pLocationTest) {
	double dPriority = -1.0;

	if(pLocationTest->getKnown(iPopulationToControl) && pLocationTest->getDetectableCoverageProportion(iPopulationToControl) > 0.0) {
		dPriority = 1.0;
	}

	if(bPriorityBasedControl) {
		switch(iPrioritySystem) {
		case PRI_RANDOM:
			//Do nothing, shuffling will sort this out later
			break;
		case PRI_RASTER:
			{
				double rPriority = prPriorities->getValue(pLocationTest->xPos, pLocationTest->yPos);
				if(dPriority > 0.0) {
					dPriority = rPriority;
				}
			}
			break;
		default:
			//ERROR: got here somehow...
			dPriority = -1;
			break;
		}
	}

	return dPriority;
}

double InterControlCull::dGetControlRadius(LocationMultiHost *pLocationCentre) {
	switch(cisControlIntensity) {
	case CIS_BLANKET:
		return dCI_BlanketRadius;
		break;
	case CIS_FROMRASTER:
		return (*prCI_RasterRadius)(pLocationCentre->xPos, pLocationCentre->yPos);
		break;
	default:
		//ERROR:
		return -1.0;
		break;
	}
}

InterControlCull::ControlType InterControlCull::ctGetControlType(LocationMultiHost *pLocationCentre) {
	switch(cmsControlMethod) {
	case CMS_BLANKET:
		return ctControlMethodBlanketType;
		break;
	case CMS_RASTER:
		return (*prcmCM_Raster)(pLocationCentre->xPos, pLocationCentre->yPos);
		break;
	default:
		//ERROR:
		return CT_NONE;
		break;
	}
}

void InterControlCull::updateSprayStatus() {

	double minNextUpdateTime = world->timeEnd + 1e30;

	double currentTime = world->time;

	int nModified = 0;

	//PR
	if (sprayPRProps.decays) {
		for (auto it = activeSprayPR.begin(); it != activeSprayPR.end(); /*erasing during iteration*/) {
			if (it->second.timeNextDecay <= currentTime) {
				double newEffectiveness = it->second.currentEffectiveness * sprayPRProps.decayFactor;
				if (newEffectiveness < sprayPRProps.minEffectiveness) {
					newEffectiveness = 0.0;
				}

				it->second.timeNextDecay += sprayPRProps.decayPeriod;
				it->second.currentEffectiveness = newEffectiveness;
				it->first->sprayPR(it->second.currentEffectiveness, it->second.dMaxAllowedHost, iPopulationToControl);//TODO: Put population into the spray data so that can spray on a per population basis

				++nModified;

				if (newEffectiveness >= sprayPRProps.minEffectiveness) {
					++it;
				} else {
					it = activeSprayPR.erase(it);
				}

			} else {
				++it;
			}
		}

		for (const auto &sprayInfoPR : activeSprayPR) {
			minNextUpdateTime = min(minNextUpdateTime, sprayInfoPR.second.timeNextDecay);
		}

	}

	//AS
	if (sprayASProps.decays) {
		for (auto it = activeSprayAS.begin(); it != activeSprayAS.end(); /*erasing during iteration*/) {
			if (it->second.timeNextDecay <= currentTime) {
				double newEffectiveness = it->second.currentEffectiveness * sprayASProps.decayFactor;
				if (newEffectiveness < sprayASProps.minEffectiveness) {
					newEffectiveness = 0.0;
				}

				it->second.timeNextDecay += sprayASProps.decayPeriod;
				it->second.currentEffectiveness = newEffectiveness;
				it->first->sprayAS(it->second.currentEffectiveness, it->second.dMaxAllowedHost, iPopulationToControl);//TODO: Put population into the spray data so that can spray on a per population basis
				
				++nModified;

				if (newEffectiveness >= sprayASProps.minEffectiveness) {
					++it;
				} else {
					it = activeSprayAS.erase(it);
				}
			} else {
				++it;
			}
		}

		for (const auto &sprayInfoAS : activeSprayAS) {
			minNextUpdateTime = min(minNextUpdateTime, sprayInfoAS.second.timeNextDecay);
		}

	}

	if (nModified > 0) {
		//TODO: Would be nice to be able to do this more efficiently
		//Currently nModified is redundant, as this function is not called unless a something is scheduled to change
		//but it can serve as a placeholder of recording which locations to update if using the faster update queue method.
		world->bFlagNeedsFullRecalculation();
	}

	timeNextSprayDecay = minNextUpdateTime;

	return;
}

void InterControlCull::applyScriptedControls() {

	double lastApplicationTime = timeDoneScriptedControl;
	double nextUnappliedTime = world->timeEnd + 1e30;

	int nModified = 0;

	//do all scripted controls between timeDoneScriptedControl and world->time
	for (const auto &scriptedControl : scriptedControls) {
		//If a control has not yet been applied, and it should have been done by now:
		if (scriptedControl.applicationTime >= timeDoneScriptedControl && scriptedControl.applicationTime <= world->time) {
			lastApplicationTime = max(lastApplicationTime, scriptedControl.applicationTime);
			//TODO: Apply scripted control
			for (auto loc : scriptedControl.locations) {
				dControlLocation(loc, loc, scriptedControl.type, scriptedControl.targetPopulation, scriptedControl.effectiveness, 1.0);
				++nModified;
			}
		} else {
			//If a scripted control is in the future, and we haven't done it yet, it could be the next thing we want to apply:
			if (scriptedControl.applicationTime > world->time && scriptedControl.applicationTime > timeDoneScriptedControl) {
				nextUnappliedTime = min(nextUnappliedTime, scriptedControl.applicationTime);
			}
		}
	}

	timeDoneScriptedControl = lastApplicationTime;
	timeNextScriptedControl = nextUnappliedTime;

	if (nModified > 0) {
		//TODO: Would be nice to be able to do this more efficiently
		//Currently nModified is redundant, as this function is not called unless a something is scheduled to change (except in the pathological situation that there was no host in all specified locations...)
		//but it can serve as a placeholder of recording which locations to update if using the faster update queue method.
		world->bFlagNeedsFullRecalculation();
	}

	return;
}

double InterControlCull::dControlLocation(LocationMultiHost *pLocationToActuallyApplyControl, LocationMultiHost *pLocationCentre, ControlType controlType, int iTargetPopulation, double dEffectiveness, double dMaxAllowedHost) {

	double dWorkDone = 0.0;

	switch(controlType) {
	case CT_CULL:
		dWorkDone = pLocationToActuallyApplyControl->cull(dEffectiveness, dMaxAllowedHost, iTargetPopulation);
		break;
	case CT_ROGUE:
		dWorkDone = pLocationToActuallyApplyControl->rogue(dEffectiveness, dMaxAllowedHost, iTargetPopulation);
		break;
	case CT_SPRAYAS:
		dWorkDone = pLocationToActuallyApplyControl->sprayAS(dEffectiveness, dMaxAllowedHost, iTargetPopulation);
		if (sprayASProps.decays) {
			double timeThisDecay = world->time + sprayASProps.decayPeriod;
			timeNextSprayDecay = min(timeNextSprayDecay, timeThisDecay);
			activeSprayAS[pLocationToActuallyApplyControl] = SprayEffect(dEffectiveness, dMaxAllowedHost, timeThisDecay);
		}
		break;
	case CT_SPRAYPR:
		dWorkDone = pLocationToActuallyApplyControl->sprayPR(dEffectiveness, dMaxAllowedHost, iTargetPopulation);
		if (sprayPRProps.decays) {
			double timeThisDecay = world->time + sprayPRProps.decayPeriod;
			timeNextSprayDecay = min(timeNextSprayDecay, timeThisDecay);
			activeSprayPR[pLocationToActuallyApplyControl] = SprayEffect(dEffectiveness, dMaxAllowedHost, timeThisDecay);
		}
		break;
	case CT_SPRAYBOTH:
		dWorkDone = pLocationToActuallyApplyControl->sprayAS(dEffectiveness, dMaxAllowedHost, iTargetPopulation);
		if (sprayASProps.decays) {
			double timeThisDecay = world->time + sprayASProps.decayPeriod;
			timeNextSprayDecay = min(timeNextSprayDecay, timeThisDecay);
			activeSprayAS[pLocationToActuallyApplyControl] = SprayEffect(dEffectiveness, dMaxAllowedHost, timeThisDecay);
		}
		dWorkDone = pLocationToActuallyApplyControl->sprayPR(dEffectiveness, dMaxAllowedHost, iTargetPopulation);
		if (sprayPRProps.decays) {
			double timeThisDecay = world->time + sprayPRProps.decayPeriod;
			timeNextSprayDecay = min(timeNextSprayDecay, timeThisDecay);
			activeSprayPR[pLocationToActuallyApplyControl] = SprayEffect(dEffectiveness, dMaxAllowedHost, timeThisDecay);
		}
	case CT_NONE:
		//Do nothing
		break;
	default:
		//ERROR:
		//...
		break;
	}

	return dWorkDone;

}

InterControlCull::ControlType InterControlCull::ctNameToCT(const char *szControlName) {

	ControlType ctRet = CT_MAX;

	for(int iCT=0; iCT<CT_MAX; iCT++) {
		if(strcmp(szControlName, ControlTypeNames[iCT]) == 0) {
			ctRet = (ControlType)iCT;
		}
	}

	return ctRet;

}

const char *InterControlCull::szCTToName(ControlType ctType) {
	return ControlTypeNames[ctType];
}

InterControlCull::ControlMethodSpecification InterControlCull::cmsNameToCMS(const char *szMethodSpecificationName) {

	ControlMethodSpecification ctRet = CMS_MAX;

	for(int iCMS=0; iCMS<CMS_MAX; iCMS++) {
		if(strcmp(szMethodSpecificationName, ControlMethodSpecificationNames[iCMS]) == 0) {
			ctRet = (ControlMethodSpecification)iCMS;
		}
	}

	return ctRet;

}

const char *InterControlCull::szCMSToName(ControlMethodSpecification cmsType) {
	return ControlMethodSpecificationNames[cmsType];
}

InterControlCull::ControlIntensitySpecification InterControlCull::cisNameToCIS(char *szControlName) {

	ControlIntensitySpecification ctRet = CIS_MAX;

	for(int iCIS=0; iCIS<CIS_MAX; iCIS++) {
		if(strcmp(szControlName, ControlIntensitySpecificationNames[iCIS]) == 0) {
			ctRet = (ControlIntensitySpecification)iCIS;
		}
	}

	return ctRet;

}

const char *InterControlCull::szCISToName(ControlIntensitySpecification ctType) {
	return ControlIntensitySpecificationNames[ctType];
}

int InterControlCull::finalAction() {
	//printf("ControlCull::finalAction()\n");

	return 1;
}

void InterControlCull::writeSummaryData(FILE *pFile) {
	//Output area culled, no. trees culled(by densities, densities infectious, cost of culling
	fprintf(pFile, "%s:\n",InterName);

	if(enabled) {
		fprintf(pFile, "Cost %12.12f\n\n", totalCost);
	} else {
		fprintf(pFile, "Disabled\n\n");
	}

	return;
}
