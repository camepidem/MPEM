//Landscape.cpp

//Largest class, also worst organised. Should be split into Simulation and Landscape classes

#include "stdafx.h"
#include "cfgutils.h"
#include "myRand.h"
#include <limits.h>
#include "RNGMT.h"
#include "Raster.h"
#include "LocationMultiHost.h"
#include "Population.h"
#include "PopulationCharacteristics.h"
#include "RateManager.h"
#include "DispersalManager.h"
#include "DispersalKernel.h"
#include "ListManager.h"
#include "SummaryDataManager.h"
#include "InterventionManager.h"
#include "InterOutputDataRaster.h"
#include "SimulationProfiler.h"
#include "ProfileTimer.h"
#include "Landscape.h"
#include <chrono>
#include "json/json.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

//Constructor:
Landscape::Landscape(int RunNo) {

	bAborted = 0;
	bSaveData = 0;

	//Run number for multiple runs:
	setRunNo(RunNo);

	//Unique action flag:
	uniqueActionIndex = 1;

	//Memory Housekeeping:
	summaryData			= NULL;
	listManager			= NULL;
	dispersal			= NULL;
	rates				= NULL;
	interventionManager	= NULL;
	profiler			= NULL;
	header				= NULL;
}

//Destructor:
Landscape::~Landscape() {
	
	if(summaryData != NULL) {
		delete summaryData;
		summaryData = NULL;
	}
	//printf("Killed summaryData\n");
	
	if(listManager != NULL) {
		delete listManager;
		listManager = NULL;
	}
	//printf("Killed listManager\n");
	
	//Delete Locations
	int nElements = locationArray.size();
	for (int i = 0; i < nElements; i++) {
		if (locationArray[i] != NULL) {
			delete locationArray[i];
			locationArray[i] = NULL;
		}
	}
	//printf("Killed locationArray\n");
	
	if(dispersal != NULL) {
		delete dispersal;
		dispersal = NULL;
	}
	//printf("Killed dispersal\n");
	
	if(rates != NULL) {
		delete rates;
		rates = NULL;
	}
	//printf("Killed rates\n");

	if(interventionManager != NULL) {
		delete interventionManager;
		interventionManager = NULL;
	}

	if (profiler != NULL) {
		delete profiler;
		profiler = NULL;
	}

	if (header != NULL) {
		delete header;
		header = NULL;
	}

	//Delete the population/popcharacteristics data
	Population::destroyPopulations();
	//printf("Killed PopulationCharacteristics\n");
}

int Landscape::abortSimulation(int saveData) {
	//Stop running at next opportunity
	run = 0;
	bSaveData = saveData;

	auto end = std::chrono::system_clock::now();
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);

	char fName[N_MAXFNAMELEN];
	sprintf(fName, "%sAbortLog%s", szPrefixOutput, szSuffixTextFile);

	char simTime[_N_MAX_STATIC_BUFF_LEN];
	sprintf(simTime, "%.6f", time);

	//Logging
	Json::Value root;
	root["Date"] = std::string(std::ctime(&end_time));
	root["SimulationTime"] = std::string(simTime);
	root["SavedData"] = bSaveData;

	ofstream abortFile;
	abortFile.open(fName);
	abortFile << root;
	abortFile.close();

	//Break out of simulating loop:
	interventionManager->abortSimulation();

	//Log that simulation was aborted:
	return bAborted = 1;
}

//Initialization Methods:

int Landscape::init(const char *version) {

	//Initialise the utilities:
	initUtilities(version);

	//create the world
	try {
		run = makeLandscape();
	}
	catch (std::exception &e) {
		std::cerr << "ERROR: Initialisation failed with exception: " << e.what() << std::endl;
		run = 0;
	}

	return run;

}

int Landscape::initUtilities(const char *version) {

	szMasterFileName = "M_MASTER.txt";

	szKeyFileName = "M_KEYFILE.txt";

	szPrefixLandscapeRaster = "L_";

	szBaseDensityName = "HOSTDENSITY";

	szSuffixTextFile = ".txt";

	profiler = new SimulationProfiler();

	//Should reading provide information on all available options
	if(!readIntFromCfg(szMasterFileName,"GiveKeys",&bGiveKeys)) {
		bGiveKeys = 0;
	}

	if(bGiveKeys) {

		//prevent simulation from running as will be breaking all the reading code:
		run = 0;

		FILE *pKeyFile = fopen(szKeyFileName,"w");
		if(pKeyFile) {
			fprintf(pKeyFile,"//This file contains a listing of the key options that can be used in the master file\n//To disable this function and use the simulator remove GiveKeys 1 from the Master file\n//\n//=InterventionName= gives the name of the section to which the keys belong\n//KeyName* indicates optional\n//Indentation indicates that a key is only used if its parent key is enabled\n");
			fprintf(pKeyFile,"//Version: %s\n", version);
			fclose(pKeyFile);
		} else {
			printf("ERROR: Unable to open keys file for writing\n");
		}
	}

	initCFG(this, version);

	return 0;
}

int Landscape::makeRNG() {
	//Seed global RNG:
	char sULRange[_N_MAX_STATIC_BUFF_LEN];
	sprintf(sULRange, "[0, %lu]", ULONG_MAX);
	RNGSeed = 0;
	if(!readValueToProperty("RNGSeed",&RNGSeed,-1, sULRange)) {
		RNGSeed = 0;
	}
	RNGSeed = vSeedRNG(RNGSeed,runNo);

	printk("RNG Seed %lu\n",RNGSeed);

	return 1;
}

int Landscape::makeLandscape() {

	bInitialisationOnTrack = 1;

	//Initialise static references:
	LocationMultiHost::setLandscape(this);
	Population::setLandscape(this);

	RateManager::setLandscape(this);
	DispersalManager::setLandscape(this);
	InterventionManager::setLandscape(this);
	
	//Timekeeping:
	profiler->initialisationTimer.start();

	//Read information from configuration file:
	readMaster();

	//Check all raster data supplied is correctly formatted:
	verifyData();

	//Set up basic data structures:
	initDataStructures();

	//Read Raster Data
	readRasters();

	//Read kernel data
	makeKernel();

	//Set up evolution lists
	makeEvolutionStructure();

	//Create Intervention structure
	makeInterventionStructure();

	profiler->initialisationTimer.stop();

	return bInitialisationOnTrack || bGiveKeys;
}

//Data reading Methods:
int Landscape::readMaster() {
	if(checkPresent(szMasterFileName)) {

		printk("Reading Master Data:\n\n");

		//Read Simulation Data
		printk("Reading Master Simulation Data: ");

		setCurrentWorkingSection("MASTER");

		setCurrentWorkingSubection("Random Number Generator Configuration", CM_APPLICATION);

		//seed the random number generator
		makeRNG();

		setCurrentWorkingSubection("Simulation Configuration", CM_SIM);

		timeStart = 0.0;
		//Configurable start time:
		readValueToProperty("SimulationStartTime", &timeStart, -1, "");

		//start the clock
		time = timeStart;

		double simLength = 1.0;
		readValueToProperty("SimulationLength", &simLength, 1, ">0");

		if (simLength <= 0.0) {
			reportReadError("ERROR: SimulationLength (%f) must be > 0.0\n", simLength);
		}

		timeEnd = timeStart + simLength;

		if (timeEnd <= timeStart && simLength > 0.0) {
			reportReadError("ERROR: You have managed to choose a SimulationStartTime (%g) so large relative to SimulationLength (%g) that they cannot be added at double precision.\n", timeStart, simLength);
		}

		setCurrentWorkingSubection("Model Configuration", CM_MODEL);

		bTrackInoculumDeposition = 0;
		readValueToProperty("TrackInoculumDeposition", &bTrackInoculumDeposition, -1, "[0,1]");

		bTrackInoculumExport = 0;
		readValueToProperty("TrackInoculumExport", &bTrackInoculumExport, -1, "[0,1]");

		sprintf(szFullModelType,"SIR");
		readValueToProperty("Model",szFullModelType, 1, "#ModelString#");

		makeUpper(szFullModelType);

		//Have the populations create the model forms from the given specification:
		Population::initPopulations(szFullModelType);

		LocationMultiHost::initLocations();

		if(!bReadingErrorFlag || bGiveKeys) {
			printk("Complete\n\n");
		} else {
			printk("Failed!\n\nAborting\n\n");
			bInitialisationOnTrack = 0;
			return 0;
		}


		//Read header variables:
		header = new RasterHeader<int>();
		if(!bIsKeyMode()) {
			
			printk("Reading Master Header Data: ");
			char szTemp[_N_MAX_STATIC_BUFF_LEN];
			sprintf(szTemp,"%s%d_%s%s",szPrefixLandscapeRaster,0,szBaseDensityName,szSuffixTextFile);

			if(!header->init(szTemp)) {
				reportReadError("ERROR: Unable to read primary landscape file header %s\n", szTemp);
			}

			//Set nodata to a consistent invalid value
			header->NODATA = -1;

		}

		if(!bReadingErrorFlag || bGiveKeys) {
			printk("Complete\n\n");
		} else {
			printk("Failed!\n\nAborting\n\n");
			bInitialisationOnTrack = 0;
			return 0;
		}


		if(!bReadingErrorFlag || bGiveKeys) {
			printk("Reading Master: Complete\n\n");
			return 1;
		} else {
			bInitialisationOnTrack = 0;
			return 0;
		}


	} else {
		printk("Unable to find %s!\n\nAborting\n\n",szMasterFileName);
		bInitialisationOnTrack = 0;
		return 0;
	}
}

int Landscape::verifyData() {

	//Do nothing if generating keyfile:
	if(bGiveKeys) {
		return 1;
	}

	if(bInitialisationOnTrack) {
		printk("Verifying data:\n\n");
		//check each required file is present and header is correct
		int bS = 1;

		checkInitialDataRequired();
		//Check host density present:
		char *fName = new char[_N_MAX_DYNAMIC_BUFF_LEN];
		for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
			sprintf(fName,"%s%d_%s%s",szPrefixLandscapeRaster,i,szBaseDensityName,szSuffixTextFile);//TODO: Generalised function to make raster names...
			if(!checkHeader(fName)) {
				reportReadError("ERROR: Unable to find required initial data file: %s\n",fName);
				bS = 0;
			}
		}

		//Check initial host allocations:
		if(iInitialDataRequired != iInitialDataUnnecessary) {
			for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
				for(int j=0; j<Population::pGlobalPopStats[i]->nClasses; j++) {
					sprintf(fName,"%s%d_%s%s",szPrefixLandscapeRaster,i,Population::pGlobalPopStats[i]->pszClassNames[j],szSuffixTextFile);
					if(!checkHeader(fName)) {
						reportReadError("ERROR: Unable to find required initial data file: %s\n",fName);
						bS = 0;
					}
				}
			}
		}

		int nRet;
		if(bS) {
			printk("Required Input data check Successful\n\n");
			nRet = 1;
		} else {
			printk("Required Input data check Failed\n\n");
			nRet = 0;
		}
		printk("Checking: Optional Data Input:\n\n");

		const char *optionals[] = {"INFECTIVITY","SUSCEPTIBILITY","SPRAYAS","SPRAYPR","SEARCH","KNOWN"};
		const int nOptionals = 6;

		for(int i=-1; i<PopulationCharacteristics::nPopulations; i++) {
			for(int j=0; j<nOptionals; j++) {
				if(i>=0) {
					sprintf(fName,"%s%d_%s%s",szPrefixLandscapeRaster,i,optionals[j],szSuffixTextFile);
				} else {
					sprintf(fName,"%sA_%s%s",szPrefixLandscapeRaster,optionals[j],szSuffixTextFile);
				}
				if(checkPresent(fName)) {
					if(!checkHeader(fName)) {
						reportReadError("ERROR: Unable to find required initial data file: %s\n",fName);
						bS = 0;
					}/* else {
						printf("DEBUG: Found %s\n",fName);
					}*/
				}
			}
		}

		if(bS) {
			printk("Optional Input data check Successful\n\n");
			nRet = 1;
		} else {
			printk("Optional Input data check Failed\n\n");
			nRet = 0;
		}

		delete [] fName;

		if(!nRet) {
			bInitialisationOnTrack = 0;
		}
		return nRet;
	} else {
		return bInitialisationOnTrack;
	}

}

int Landscape::initDataStructures() {

	if(bInitialisationOnTrack) {

		//Summary Data:
		summaryData = new SummaryDataManager();

		SummaryDataManager::init(*header);

		if(!bIsKeyMode()) {
			//Lists
			listManager = new ListManager();

			//Make Landscape Objects
			int nElements = header->nRows*header->nCols;
			locationArray.resize(nElements);
			//Just to be safe, set everything to NULL:
			for(int i = 0; i < nElements; i++) {
				locationArray[i] = NULL;
			}
		}

	}

	return 1;
}

int Landscape::readRasters() {
	if(bInitialisationOnTrack && !bGiveKeys) {
		printf("Reading Data Files\n\n");

		//Read other files based of information in master
		readDensities();

		//Read remaining data and place in locations
		readHostAllocations();

		if(bInitialisationOnTrack) {//Only read this if locations and populations have been created successfully
			//TODO: Literally the stuff of nightmares
			//Optional data input:
			char *fName = new char[_N_MAX_DYNAMIC_BUFF_LEN];
			const char *optionals[] = {"INFECTIVITY","SUSCEPTIBILITY","SPRAYAS","SPRAYPR","SEARCH","KNOWN"};
			const int nDoubleOptionals = 4;
			const int nIntOptionals = 2;
			typedef int (LocationMultiHost::*LocFnPtrDbl)(double, int);
			typedef int (LocationMultiHost::*LocFnPtrInt)(int, int);
			vector<LocFnPtrDbl> pFnsD(nDoubleOptionals);
			pFnsD[0] = &LocationMultiHost::scaleInfBase;
			pFnsD[1] = &LocationMultiHost::scaleSusBase;
			pFnsD[2] = &LocationMultiHost::setInfSpray;
			pFnsD[3] = &LocationMultiHost::setSusSpray;
			vector<LocFnPtrInt> pFnsI(nIntOptionals);
			pFnsI[0] = &LocationMultiHost::setSearch;
			pFnsI[1] = &LocationMultiHost::setKnown;


			for(int i=-1; i<PopulationCharacteristics::nPopulations; i++) {
				for(int j=0; j<nDoubleOptionals; j++) {
					if(i>=0) {
						sprintf(fName,"%s%d_%s%s",szPrefixLandscapeRaster,i,optionals[j],szSuffixTextFile);
					} else {
						sprintf(fName,"%sA_%s%s",szPrefixLandscapeRaster,optionals[j],szSuffixTextFile);
					}
					if(checkPresent(fName)) {
						readRasterToProperty(fName,i,pFnsD[j]);
					}
				}
				for(int j=0; j<nIntOptionals; j++) {
					if(i>=0) {
						sprintf(fName,"%s%d_%s%s",szPrefixLandscapeRaster,i,optionals[nDoubleOptionals + j],szSuffixTextFile);
					} else {
						sprintf(fName,"%sA_%s%s",szPrefixLandscapeRaster,optionals[nDoubleOptionals + j],szSuffixTextFile);
					}
					if(checkPresent(fName)) {
						readRasterToProperty(fName,i,pFnsI[j]);
						//printf("DEBUG: Read %s\n",fName);
					}
				}
			}

			delete [] fName;
		}

	}

	return bInitialisationOnTrack;

}

int Landscape::readDensities() {

	if(bInitialisationOnTrack && !bGiveKeys) {

		int nPopulations = PopulationCharacteristics::nPopulations;

		//Names of files:
		char **pDensityFileNamesArray = new char*[nPopulations];

		//Active files:
		vector<FILE *> pFileArray(nPopulations);

		//Reading buffer:
		char *pPtr;
		char **pCurrent = new char*[nPopulations];
		char **szBuff = new char*[nPopulations];
		for(int i=0; i<nPopulations; i++) {
			szBuff[i] = new char[_N_MAX_DYNAMIC_BUFF_LEN];
		}
		char *szTemp = new char[64];
		size_t nL;

		//Flag for if found any densities positive over all populations at current locations:
		int bAnyPositive = 0;

		//Densities of populations in current location:
		vector<double> tempDensityArray(nPopulations);
		for(int i=0; i<nPopulations; i++) {
			tempDensityArray[i] = 0.0;
		}

		//Current raster coordinates:
		int x = 0, y = 0;

		//Compose file names:
		for(int i=0; i<nPopulations; i++) {
			pDensityFileNamesArray[i] = new char[N_MAXFNAMELEN];
			sprintf(pDensityFileNamesArray[i],"%s%d_%s%s",szPrefixLandscapeRaster,i,szBaseDensityName,szSuffixTextFile);
			//printf("DEBUG: Trying file: \'%s\'\n", pDensityFileNamesArray[i]);
			pFileArray[i] = fopen(pDensityFileNamesArray[i],"rb");
			if(!pFileArray[i]) {
				reportReadError("ERROR: Unable to open file %s for reading\n",pDensityFileNamesArray[i]);
				bInitialisationOnTrack = 0;
				return 0;
			}
		}

		for(int i=0; i<nPopulations; i++) {
			//Strip headers:
			for(int j=0; j<6; j++) {
				if (!fgets(szBuff[0], _N_MAX_DYNAMIC_BUFF_LEN, pFileArray[i])) {
					reportReadError("ERROR: Unable to read header line %d from file %s\n", j, pDensityFileNamesArray[i]);
				}
			}
		}

		//Main reading
		while(y < header->nRows) {

			//Read line from each file:
			for(int i=0; i<nPopulations; i++) {
				if (!fgets(szBuff[i], _N_MAX_DYNAMIC_BUFF_LEN, pFileArray[i])) {
					reportReadError("ERROR: Unable to read body line %d from file %s\n", y, pDensityFileNamesArray[i]);
				}
				//Strip off newline:
				pPtr = strpbrk(szBuff[i], "\r\n");
				if(pPtr) {
					//Ensure line ends with a space and a null char
					//bit risky to append 2 characters but incredibly unlikely to actually be anywhere near the end of the buffer
					pPtr[0] = ' ';
					pPtr[1] = '\0';
				} else {
					fprintf(stderr, "ERROR: In file %s line %d too long\n", pDensityFileNamesArray[i], y);
					bInitialisationOnTrack = 0;
					return 0;				/* no CR or NL means line not read in fully */
				}

				//Point current to start of line buffer:
				pCurrent[i] = szBuff[i];

			}

			while(x < header->nCols) {
				//Get each file to get the next value:
				for(int i=0; i<nPopulations; i++) {
					//Move forward until we find some non-whitespace data
					while ((pCurrent[i][0] == ' ' || pCurrent[i][0] == '\t') && pCurrent[i][0] != '\0') {
						pCurrent[i]++;
					}
					//Get position of the next space
					pPtr = strpbrk(pCurrent[i]," \t");
					if(pPtr == NULL) {
						reportReadError("ERROR: At x=%d, y=%d in file %s Could not find next value\n",x,y,pDensityFileNamesArray[i]);
						bInitialisationOnTrack = 0;
						return 0;
					}
					//Copy from pCurrent up to pPtr into szTemp
					nL = pPtr - pCurrent[i];
					strncpy(szTemp, pCurrent[i], nL);
					//strncpy does not zero terminate...
					szTemp[nL] = '\0';
					//Point current position to next space:
					pCurrent[i] = pPtr;

					tempDensityArray[i] = atof(szTemp);
				}

				//Check the data:
				for(int i=0; i<nPopulations; i++) {
					if(tempDensityArray[i] > 0.0) {
						if(tempDensityArray[i] > 1.0) {
							reportReadError("ERROR: At x=%d, y=%d in file %s Density Value: %f > 1.0\n",x,y,pDensityFileNamesArray[i],tempDensityArray[i]);
							bInitialisationOnTrack = 0;
							return 0;
						}
						bAnyPositive++;
					}
				}

				//Use the data if necessary:
				if(bAnyPositive) {

					locationArray[x+y*header->nCols] = new LocationMultiHost(x, y);

					if(!locationArray[x+y*header->nCols]->readDensities(&tempDensityArray[0])) {
						bInitialisationOnTrack = 0;
						return 0;
					}

					bAnyPositive = 0;
				}

				//Have taken a value, so increment x position:
				x++;

			}

			if(x == header->nCols) {
				//Reached end of the line:
				x = 0;
				y++;
			}

		}

		if(y == header->nRows) {
			//Have now read in all lines successfully:

			//Close files:
			for(int i=0; i<nPopulations; i++) {
				if(pFileArray[i]) {
					fclose(pFileArray[i]);
				}
			}

			//Deallocate buffers:
			for(int i=0; i<nPopulations; i++) {
				delete [] pDensityFileNamesArray[i];
				delete [] szBuff[i];
			}

			delete [] pDensityFileNamesArray;
			delete [] pCurrent;
			delete [] szBuff;
			delete [] szTemp;

			return 1;
		} else {
			//Seem to be here without finishing, so something went wrong...
			reportReadError("ERROR: Found %d lines expected %d\n", y, header->nRows);
			bInitialisationOnTrack = 0;
			return 0;
		}

	} else {
		return 1;
	}
}

//These reading functions are so horrible as once, long ago, there was not enough memory to just read in all of the raster files entirely at once, 
//so all the files have to be read one element at a time, never reading any one in its entireity. 
//TODO: Perhaps now it would be far better to streamline this.
int Landscape::readHostAllocations() {	

	if(bInitialisationOnTrack && !bGiveKeys) {
		//TODO: Facility for not specifying initial data:
		//-nothing: All susceptible
		//-some:	All zero except what is specified
		//-alternative:all susceptible except what is specified?

		if(iInitialDataRequired != iInitialDataUnnecessary) {
			double **tempHostAllocationArrays = new double*[PopulationCharacteristics::nPopulations];

			for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
				tempHostAllocationArrays[i] = new double[Population::pGlobalPopStats[i]->nClasses];

				tempHostAllocationArrays[i][0] = 1.0;
				for(int j=1; j<Population::pGlobalPopStats[i]->nClasses; j++) {
					tempHostAllocationArrays[i][j] = 0.0;
				}

			}

			int nPopulations = PopulationCharacteristics::nPopulations;

			//Names of files:
			char ***pHostFileNames = new char**[nPopulations];


			//Active files:
			vector<vector<FILE *>> pFileArray(nPopulations);
			for(int i=0; i<nPopulations; i++) {
				pFileArray[i].resize(Population::pGlobalPopStats[i]->nClasses);
			}

			//Reading buffer:
			char *pPtr;
			char ***pCurrent = new char**[nPopulations];
			char ***szBuff = new char**[nPopulations];
			for(int i=0; i<nPopulations; i++) {
				pCurrent[i] = new char*[Population::pGlobalPopStats[i]->nClasses];
				szBuff[i] = new char*[Population::pGlobalPopStats[i]->nClasses];
				for(int j=0; j<Population::pGlobalPopStats[i]->nClasses; j++) {
					szBuff[i][j] = new char[_N_MAX_DYNAMIC_BUFF_LEN];
				}
			}

			char *szTemp = new char[64];
			size_t nL;

			//Flag for if found any densities positive over all populations at current locations:
			int bAnyPositive = 0;

			//Current raster coordinates:
			int x = 0, y = 0;

			//Compose file names:
			for(int i=0; i<nPopulations; i++) {
				pHostFileNames[i] = new char*[Population::pGlobalPopStats[i]->nClasses];
				for(int j=0; j<Population::pGlobalPopStats[i]->nClasses; j++) {
					pHostFileNames[i][j] = new char[128];
					sprintf(pHostFileNames[i][j],"%s%d_%s%s",szPrefixLandscapeRaster,i,Population::pGlobalPopStats[i]->pszClassNames[j],szSuffixTextFile);
					//printf("DEBUG: Trying file: \'%s\'\n", pHostFileNames[i][j]);
					pFileArray[i][j] = fopen(pHostFileNames[i][j],"rb");
					if(!pFileArray[i][j]) {
						reportReadError("ERROR: Unable to open file %s for reading\n",pHostFileNames[i][j]);
						bInitialisationOnTrack = 0;
						return 0;
					}
				}
			}

			for(int i=0; i<nPopulations; i++) {
				for(int j=0; j<Population::pGlobalPopStats[i]->nClasses; j++) {
					//Strip headers:
					for(int k=0; k<6; k++) {
						if (!fgets(szBuff[0][0], _N_MAX_DYNAMIC_BUFF_LEN, pFileArray[i][j])) {
							reportReadError("ERROR: Unable to read header line %d from file %s\n", k, pHostFileNames[i][j]);
						}
					}
				}
			}

			//Main reading
			while(y < header->nRows) {

				//Read line from each file:
				for(int i=0; i<nPopulations; i++) {
					for(int j=0; j<Population::pGlobalPopStats[i]->nClasses; j++) {
						if (!fgets(szBuff[i][j], _N_MAX_DYNAMIC_BUFF_LEN, pFileArray[i][j])) {
							reportReadError("ERROR: Unable to read body line %d from file %s\n", y, pHostFileNames[i][j]);
						}
						//Strip off newline:
						pPtr = strpbrk(szBuff[i][j], "\r\n");
						if(pPtr) {
							//Ensure line ends with a space and a null char
							//bit risky to append 2 characters but incredibly unlikely to actually be anywhere near the end of the buffer
							pPtr[0] = ' ';
							pPtr[1] = '\0';
						} else {
							fprintf(stderr, "ERROR: In file %s line %d too long\n", pHostFileNames[i][j], y);
							bInitialisationOnTrack = 0;
							return 0;				/* no CR or NL means line not read in fully */
						}

						//Point current to start of line buffer:
						pCurrent[i][j] = szBuff[i][j];

					}
				}

				while(x < header->nCols) {
					//Get each file to get the next value
					for(int i=0; i<nPopulations; i++) {
						for(int j=0; j<Population::pGlobalPopStats[i]->nClasses; j++) {
							//Move forward until we find some non-whitespace data
							while ((pCurrent[i][j][0] == ' ' || pCurrent[i][j][0] == '\t') && pCurrent[i][j][0] != '\0') {
								pCurrent[i][j]++;
							}
							//Get position of the next space
							pPtr = strpbrk(pCurrent[i][j]," \t");
							if(pPtr == NULL) {
								reportReadError("ERROR: At x=%d, y=%d in file %s Could not find next value\n",x,y,pHostFileNames[i][j]);
								bInitialisationOnTrack = 0;
								return 0;
							}
							//Copy from pCurrent up to pPtr into szTemp
							nL = pPtr - pCurrent[i][j];
							strncpy(szTemp, pCurrent[i][j], nL);
							//strncpy does not zero terminate...
							szTemp[nL] = '\0';
							//Point current position to next space:
							pCurrent[i][j] = pPtr;
							
							tempHostAllocationArrays[i][j] = atof(szTemp);
						}
					}

					if(locationArray[x+y*header->nCols] != NULL) {
						//If needed, allocate the data:
						locationArray[x+y*header->nCols]->populate(tempHostAllocationArrays);
					}

					//Have taken a value, so increment x position:
					x++;

				}

				if(x == header->nCols) {
					//Reached end of the line:
					x = 0;
					y++;
				}

			}

			if(y == header->nRows) {
				//Have now read in all lines successfully:

				//Close files:
				for(int i=0; i<nPopulations; i++) {
					if (pFileArray[i].size() != 0) {
						for (int j = 0; j < Population::pGlobalPopStats[i]->nClasses; j++) {
							fclose(pFileArray[i][j]);
						}
					}
				}

				//Deallocate buffers:
				for(int i=0; i<nPopulations; i++) {
					for(int j=0; j<Population::pGlobalPopStats[i]->nClasses; j++) {
						delete [] pHostFileNames[i][j];
						delete [] szBuff[i][j];
					}
					delete [] pHostFileNames[i];
					delete [] szBuff[i];
					delete [] pCurrent[i];
				}

				delete [] pHostFileNames;
				delete [] pCurrent;
				delete [] szBuff;
				delete [] szTemp;

				for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
					delete [] tempHostAllocationArrays[i];
				}

				delete [] tempHostAllocationArrays;

				if(bReadingErrorFlag) {
					reportReadError("ERROR: Some locations not successfully populated\n");
					bInitialisationOnTrack = 0;
					return 0;
				} else {
					return 1;
				}
			} else {
				//Seem to be here without finishing, so something went wrong...
				reportReadError("ERROR: Found %d lines expected %d\n", y, header->nRows);
				bInitialisationOnTrack = 0;
				return 0;
			}

		} else {
			printf("Not reading unused initial data files\n");

			//Populate from totally susceptible hosts:
			double **tempHostAllocationArrays = new double*[PopulationCharacteristics::nPopulations];

			for(int iPop = 0; iPop < PopulationCharacteristics::nPopulations; ++iPop) {
				tempHostAllocationArrays[iPop] = new double[Population::pGlobalPopStats[iPop]->nClasses];

				tempHostAllocationArrays[iPop][Population::pGlobalPopStats[iPop]->iSusceptibleClass] = 1.0;
				for(int iClass = 0; iClass < Population::pGlobalPopStats[iPop]->nClasses; ++iClass) {
					if(iClass != Population::pGlobalPopStats[iPop]->iSusceptibleClass) {
						tempHostAllocationArrays[iPop][iClass] = 0.0;
					}
				}

			}

			for(int i=0; i<header->nRows*header->nCols; i++) {
				if(locationArray[i] != NULL) {
					locationArray[i]->populate(tempHostAllocationArrays);
				}
			}

			for(int i=0; i<PopulationCharacteristics::nPopulations; i++) {
				delete [] tempHostAllocationArrays[i];
			}

			delete [] tempHostAllocationArrays;

			return 1;

		}

	} else {
		return 1;
	}
}

int Landscape::makeKernel() {

	if(bInitialisationOnTrack) {

		printk("Initializing Kernel Structure\n");

		dispersal = new DispersalManager();

		DispersalManager::init();

		Population::setDispersalManager(dispersal);

		if(!bReadingErrorFlag) {
			printk("Kernel Structure Complete\n\n");
		} else {
			printk("Kernel Structure Error\n\n");
			if(!bGiveKeys) {
				bInitialisationOnTrack = 0;
			}
		}

	}

	return bInitialisationOnTrack;
}

int Landscape::makeEvolutionStructure() {

	if(bInitialisationOnTrack) {
		printk("Initializing Evolution Structure\n");

		//Events Tracking:
		bFlagNeedsFullRecalculation();

		bForceDebugFullRecalculation = 0;

		rates = new RateManager();

		Population::setRateManager(rates);

		rates->readRateConstants();

		if(bReadingErrorFlag && !bGiveKeys) {
			bInitialisationOnTrack = 0;
		}

		//Do not initialise data while in key output mode or if have had read error:
		if(!bGiveKeys && bInitialisationOnTrack) {

			//Arrays of active Populations:
			int nPopulations = PopulationCharacteristics::nPopulations;
			vector<int> pnActive(nPopulations);
			vector<int> pnActiveFound(nPopulations);
			EventLocations.resize(PopulationCharacteristics::nPopulations);

			for(int iPop=0; iPop<nPopulations; iPop++) {
				pnActive[iPop] = Population::pGlobalPopStats[iPop]->nActiveCount;
				pnActiveFound[iPop] = 0;
				EventLocations[iPop].resize(pnActive[iPop]);
			}

			//Populate events from active populations:
			ListNode<LocationMultiHost> *pNode = listManager->locationLists[ListManager::iActiveList]->pFirst->pNext;
			while(pNode != NULL) {

				LocationMultiHost *pLoc = pNode->data;

				for(int iPop=0; iPop<nPopulations; iPop++) {
					if(pLoc->setEventIndex(iPop,pnActiveFound[iPop])) {
						EventLocations[iPop][pnActiveFound[iPop]] = pLoc;
						pnActiveFound[iPop]++;
					}
				}

				pNode = pNode->pNext;
			}

			//Check found the right number of active Populations:
			for(int iPop=0; iPop<nPopulations; iPop++) {
				if(pnActive[iPop] != pnActiveFound[iPop]) {
					reportReadError("ERROR: For population %d found %d active, registered %d in rate structure.\n",iPop,pnActiveFound[iPop],pnActive[iPop]);
				}
			}

			printk("Evolution Structure Complete\n\n");
		}
	}

	return bInitialisationOnTrack;
}

int Landscape::makeInterventionStructure() {
	
	if(bInitialisationOnTrack) {

		printk("Initializing Intervention Structure\n");

		interventionManager = new InterventionManager();

		interventionManager->init();

		if(bReadingErrorFlag) {
			bInitialisationOnTrack = 0;
		}
	}

	return bInitialisationOnTrack;

}



int Landscape::checkHeader(char *szCfgFile) {

	RasterHeader<double> testHeader;

	if(!testHeader.init(szCfgFile)) {
		printk("Checking %s: ",szCfgFile);
		printk("File reading failed\n\n");
		return 0;
	}

	if(!header->sameAs(testHeader)) {
		printk("Checking %s: ",szCfgFile);
		printk("Header mismatch\n\n");
		return 0;
	}

	return 1;

}

int Landscape::checkInitialDataRequired() {

	//Default: Read but do not save
	iInitialDataRequired = interventionManager->checkInitialDataRequired();

	return iInitialDataRequired;

}

char *Landscape::getErrorLog() {
	return getErrorMessages();
}

double Landscape::getMinimumHostScaling() {

	double dMinHostscaling = 1.0;

	for(int iPop=0; iPop<PopulationCharacteristics::nPopulations; iPop++) {
		
		double dPopHostScaling = getMinimumHostScaling(iPop);

		if(dPopHostScaling < dMinHostscaling) {
			dMinHostscaling = dPopHostScaling;
		}

	}

	return dMinHostscaling;

}

int Landscape::getUniqueActionIndex()
{
	return uniqueActionIndex++;
}

double Landscape::getMinimumHostScaling(int iPop) {

	double dMinHostscaling = 1.0;

	ListNode<LocationMultiHost> *pNode = listManager->locationLists[ListManager::iActiveList]->pFirst->pNext;
	while(pNode != NULL) {

		LocationMultiHost *pLoc = pNode->data;

		double dTestScaling = pLoc->dGetHostScaling(iPop);

		if(dTestScaling < dMinHostscaling) {
			dMinHostscaling = dTestScaling;
		}

		pNode = pNode->pNext;
	}

	return dMinHostscaling;

}

//Evolution Methods
int Landscape::prepareLandscape() {

	profiler->kernelCouplingTimer.start();

	if (bNeedsFullRecalculation) {

		++profiler->nFullRecalculations;

		profiler->fullRecalculationTimer.start();

		rates->refreshSporulationRateConstants();//TODO: Why does this happen?

		//go through all active landscape areas
		ListNode<LocationMultiHost> *pActiveNode;
		pActiveNode = listManager->locationLists[ListManager::iActiveList]->pFirst->pNext;
		while (pActiveNode != NULL) {
			//get location to give its total rate
			pActiveNode->data->getRatesFull();
			pActiveNode = pActiveNode->pNext;

			profiler->nCouplingEvents++;
		}

		rates->fullResum();

		bNeedsFullRecalculation = 0;
		bNeedsInfectionRateRecalculation = 0; //This supersedes all other things

		//If we have had to recalculate everything, then we have already accounted for any possible changes
		lastEvents.resize(0);
		recalculationQueue.resize(0);

		profiler->fullRecalculationTimer.stop();
	}

	if (bNeedsInfectionRateRecalculation) {

		++profiler->nInfectionRateRecalculations;

		profiler->infectionRateRecalculationTimer.start();

		rates->refreshSporulationRateConstants();//TODO: Why does this happen?

		//TODO: How to choose?
		int nInfectiousLocations = listManager->locationLists[ListManager::iInfectiousList]->nLength;
		int nLongestKernelRange = DispersalManager::getLongestKernelShortRange();
		int nLargestKernelArea = (2 * nLongestKernelRange + 1)*(2 * nLongestKernelRange + 1);
		int nMaximumPossibleCellsToRecalculate = nInfectiousLocations * nLargestKernelArea;

		int nCellsInLandscape = header->nCols*header->nRows;

		int nPopulatedCells = listManager->locationLists[ListManager::iActiveList]->nLength;

		double inhabitedProportion = double(nPopulatedCells) / double(nCellsInLandscape);

		//Would typically expect this to be an underestimate, as epidemic more likely to be in denser regions
		double expectedCellsToRecalculate = inhabitedProportion * nMaximumPossibleCellsToRecalculate;
		
		bool bDoFullUpdate = true;

		//TODO: This threshold can be improved upon, certainly in most circumstances
		if (nMaximumPossibleCellsToRecalculate < nPopulatedCells) {
			bDoFullUpdate = false;
		}

		if (bDoFullUpdate) {
			//Just do getRatesInf() for everyone
			//go through all active landscape areas
			ListNode<LocationMultiHost> *pActiveNode = listManager->locationLists[ListManager::iActiveList]->pFirst->pNext;
			while (pActiveNode != NULL) {
				//get location to update its inf rates
				pActiveNode->data->getRatesInf();
				pActiveNode = pActiveNode->pNext;

				profiler->nCouplingEvents++;
			}

			rates->infRateResum();
		} else {
			//If there are a "small" number of infectious locations, zero out all the rates, 
			//and then only update within the neighbourhood of each infection
			//TODO: Need (?) to do something clever in the case that there might be significant overlapping regions
			rates->infRateScrub();

			int actionIndex = getUniqueActionIndex();

			//Only the lucky ones:
			ListNode<LocationMultiHost> *pInfectiousNode = listManager->locationLists[ListManager::iInfectiousList]->pFirst->pNext;
			while (pInfectiousNode != NULL) {

				LocationMultiHost *infectiousLocation = pInfectiousNode->data;

				//get all locations in region to update inf rates
				int W = header->nCols;
				int H = header->nRows;
				int nK = nLongestKernelRange;
				int xMin = max(0,                                 infectiousLocation->xPos - nK);
				int xMax = min(infectiousLocation->xPos + nK + 1, W);
				int yMin = max(0,                                 infectiousLocation->yPos - nK);
				int yMax = min(infectiousLocation->yPos + nK + 1, H);

				int H0 = yMin*W;

				for (int y = yMin; y < yMax; y++) {
					for (int x = xMin; x < xMax; x++) {
						int nLoc = H0 + x;
						if (locationArray[nLoc] != NULL) {
							if (locationArray[nLoc]->actionFlag != actionIndex) {//Not already touched
								locationArray[nLoc]->actionFlag = actionIndex;//Mark that location has been touched this round
								locationArray[nLoc]->getRatesInf();

								profiler->nCouplingEvents++;
							}
						}
					}
					H0 += W;
				}

				pInfectiousNode = pInfectiousNode->pNext;
			}

			rates->infRateResum();
		}


		bNeedsInfectionRateRecalculation = 0;

		//If we have had to recalculate all infection rates, then these changes are no longer valid:
		lastEvents.resize(0);
		recalculationQueue.resize(0);

		profiler->infectionRateRecalculationTimer.stop();

	}

	//Only update areas that can have had changes
	//Check which events have changed
	while (lastEvents.size() > 0) {

		EventInfo lastEvent = lastEvents.back();
		lastEvents.pop_back();

		//Update only locations within interaction range of the last event
		if (lastEvent.bRanged) {
			//last event was something that will change the rates of nearby regions
			//update local region

			profiler->nCouplingEvents++;

			int W = header->nCols;
			int H = header->nRows;
			int nK = DispersalManager::getKernel(lastEvent.iPop)->getShortRange();
			int xMin = max(0,                             lastEvent.pLoc->xPos - nK);
			int xMax = min(lastEvent.pLoc->xPos + nK + 1, W);
			int yMin = max(0,                             lastEvent.pLoc->yPos - nK);
			int yMax = min(lastEvent.pLoc->yPos + nK + 1, H);

			int H0 = yMin*W;
			int nLoc;

			for (int y = yMin; y < yMax; y++) {
				for (int x = xMin; x < xMax; x++) {
					nLoc = H0 + x;
					if (locationArray[nLoc] != NULL) {
						locationArray[nLoc]->getRatesAffectedByRanged(lastEvent);
					}
				}
				H0 += W;
			}

		}

	}

	//Perform full updates on locations that require it:
	while (recalculationQueue.size() > 0) {
		EventInfo recalcEvent = recalculationQueue.back();
		recalculationQueue.pop_back();

		locationArray[recalcEvent.pLoc->xPos + recalcEvent.pLoc->yPos*header->nCols]->getRatesInf(recalcEvent.iPop);

		profiler->nCouplingEvents++;
	}

	if (bForceDebugFullRecalculation) {

		vector<vector<double>> dumpedRates;
		int nPopulatedCells = listManager->locationLists[ListManager::iActiveList]->nLength;
		int nEvents = Population::pGlobalPopStats[0]->nEvents;
		dumpedRates.resize(nEvents);
		for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
			dumpedRates[iEvent].resize(nPopulatedCells);
			for (int iLoc = 0; iLoc < nPopulatedCells; ++iLoc) {
				dumpedRates[iEvent][iLoc] = rates->getRate(0, iLoc, iEvent);
			}
		}

		++profiler->nFullRecalculations;

		profiler->fullRecalculationTimer.start();

		rates->refreshSporulationRateConstants();//TODO: Why does this happen?

		//go through all active landscape areas
		ListNode<LocationMultiHost> *pActiveNode;
		pActiveNode = listManager->locationLists[ListManager::iActiveList]->pFirst->pNext;
		while (pActiveNode != NULL) {
			//get location to give its total rate
			pActiveNode->data->getRatesFull();
			pActiveNode = pActiveNode->pNext;

			profiler->nCouplingEvents++;
		}

		rates->fullResum();

		bForceDebugFullRecalculation = 0;

		//If we have had to recalculate everything, then we have already accounted for any possible changes
		lastEvents.resize(0);

		profiler->fullRecalculationTimer.stop();

		int nChanges = 0;
		for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
			for (int iLoc = 0; iLoc < nPopulatedCells; ++iLoc) {
				if (abs(dumpedRates[iEvent][iLoc] - rates->getRate(0, iLoc, iEvent)) > 1e-6) {
					if (iEvent != 0) {
						std::cerr << "Non infection event rate altered" << std::endl;
					}
					std::cerr << "Time="<< time << " Rate for event " << iEvent << " changed from " << dumpedRates[iEvent][iLoc] << " to " << rates->getRate(0, iLoc, iEvent) << " at " << EventLocations[0][iLoc]->debugGetStateString();
					nChanges++;
				}
			}
		}
		if (nChanges > 0) {
			std::cerr << "================================================================================================================================================================" << std::endl;
			std::cerr << "DANGER! Time=" << time << " had to change " << nChanges << " rates" << std::endl;
		}

	}

	profiler->kernelCouplingTimer.stop();

	return 1;

}

int Landscape::advanceLandscape(double dTimeNextInterrupt) {

	//TODO: Whole thing should be packaged into EventManager or SimulationManager

	//Get time to next event and prep the next event:
	double timeToNextEvent = rates->selectNextEvent();
	//check if simulation should have intervention before next event
	if(time + timeToNextEvent < dTimeNextInterrupt) {
		//Update time to the time of the execution
		time += timeToNextEvent;
		if(rates->executeNextEvent()) {
			//All went well:
			return 1;
		} else {
			//Have chosen an illegal event:
			profiler->nIllegalEvents++;
			//roll back time:
			time -= timeToNextEvent;
			/*printf("Illegal event selected!\n");*/
			return 0;
		}
	} else {
		//Jump to next control/intervention event or end of simulation
		//set time to time of next event
		//Don't allow time to go backwards in the case of interventions specifying times in the past...
		//TODO: Better handling of time
		if(dTimeNextInterrupt > time) {
			time = dTimeNextInterrupt;
		}
		//TODO: Now feasible with stored event information to not recalculate the event for Interventions which do not alter landscape (-need to also store t+tNext value here)
		return 0;
	}

}

int Landscape::bFlagNeedsFullRecalculation() {

	bNeedsFullRecalculation = 1;

	return bNeedsFullRecalculation;
}

int Landscape::bFlagNeedsInfectionRateRecalculation() {

	bNeedsInfectionRateRecalculation = 1;

	return bNeedsInfectionRateRecalculation;
}

int Landscape::bTrackingInoculumDeposition()
{
	return bTrackInoculumDeposition;
}

int Landscape::bTrackingInoculumExport()
{
	return bTrackInoculumExport;
}

int Landscape::setRunNo(int iRunNo) {
	runNo = iRunNo;
	sprintf(szPrefixOutput, "O_%d_",runNo);
	return runNo;
}

double Landscape::cleanWorld(int bForceTotalClean, int bPreventAllRateResubmission, int bIncrementRunNumber) {

	//TODO: Reestablish when things should resubmit rates
	//TODO: Link this better with the initial data reset...

	//DEBUG:
	//Landscape::debugCompareStateToDPCs();
	//DEBUG:

	if(bIncrementRunNumber) {
		setRunNo(runNo+1);
	}

	//Determine if it is quicker to clean all the rates arrays or to resubmit the individual rates:
	int bLocationsResubmitRates = 0;
	int nLTouched = listManager->locationLists[ListManager::iTouchedList]->nLength;
	int nLActive = listManager->locationLists[ListManager::iActiveList]->nLength;
	if(nLTouched*rates->getWorkToResubmit() < double(nLActive) ) {
		bLocationsResubmitRates = 1;
	}

	if(bPreventAllRateResubmission) {
		bLocationsResubmitRates = 0;
	}

	//Reset unique infection data:
	SummaryDataManager::resetUniqueInfections();

	ListNode<LocationMultiHost> *pNode;
	ListNode<LocationMultiHost> *pNodeNext;

	//clean locations:
	if(bForceTotalClean) {
		//use active list:
		pNode = listManager->locationLists[ListManager::iActiveList]->pFirst->pNext;
	} else {
		//use touched list:
		pNode = listManager->locationLists[ListManager::iTouchedList]->pFirst->pNext;
	}

	//TODO: Need to be very careful about how this is called w.r.t. the location scrubs especially primary infection
	//Only clean whole arrays manually if all rates are not being resubmitted anyway:
	rates->scrubRates(!bLocationsResubmitRates);

	while(pNode != NULL) {
		//get location to clean self
		pNodeNext = pNode->pNext;
		pNode->data->scrub(bLocationsResubmitRates);
		pNode = pNodeNext;
	}

	//RESET WORLD TIME:
	time = timeStart;

	DispersalManager::reset();

	//DEBUG:
	//Landscape::debugCompareStateToDPCs();
	//DEBUG:

	return summaryData->getHostQuantities(0)[0];
}

//TODO: Have extra loop round the run loop for more rigorous inclusion of bulk interventions?
//Ensures that preSimulationInterventions are meaningful?
int Landscape::simulate() {

	//Timekeeping
	profiler->overallTimer.start();

	//counters
	int ok = 1;
	breakEventLoop = 0;
	//TODO: Have every intervention get to do a pre simulation action?
	//Print the first line of DPC Data:
	interventionManager->preSimulationInterventions();
	interventionManager->manageInterventions();

	while(run) {
		
		profiler->simulationTimer.start();

		while(ok && !breakEventLoop) {
			prepareLandscape();
			profiler->totalNoEvents += ok = advanceLandscape(interventionManager->timeNextIntervention);
		}
		
		profiler->simulationTimer.stop();
		
		//have next intervention
		ok = 1;
		//Timekeeping
		profiler->interventionTimer.start();
		
		interventionManager->performIntervention();
		interventionManager->manageInterventions();
		
		profiler->interventionTimer.stop();
	}

	if(!bAborted) {
		//Clean exit:
		profiler->overallTimer.stop();

		//Have each intervention perform its final action
		interventionManager->finalInterventions();
	} else {
		//Called for abort:
		//TODO: Maybe dump critical things like RNG seed to a log file?
		if (bSaveData) {
			printk("Saving state data at simulation time=%.6f", time);
			//TODO: Pass responsibility for this up to the interventionManager?
			//Save the full raster state:
			interventionManager->pInterRaster->saveStateData(time);

			//TODO: Save the state of any interventions that need it:
			//Detection
			//Control

			//TODO: Any other state that needs to be saved?
		}
	}

	return 1;
}

int Landscape::simulateUntilTime(double dPauseTime) {

	//counters
	int ok = 1;

	if(time == timeStart) {
		//If this is the start of the simulation:
		profiler->overallTimer.start();
		//Print the first line of DPC Data:
		interventionManager->preSimulationInterventions();
		interventionManager->manageInterventions();
	}

	while(run && time < dPauseTime) {

		profiler->simulationTimer.start();
		
		while(ok) {
			prepareLandscape();
			profiler->totalNoEvents += ok = advanceLandscape(min(interventionManager->timeNextIntervention, dPauseTime));
		}
		
		profiler->simulationTimer.stop();

		//have next intervention
		ok = 1;

		profiler->interventionTimer.start();
		
		if(time == interventionManager->timeNextIntervention) {
			interventionManager->performIntervention();
			interventionManager->manageInterventions();
		}
		
		profiler->interventionTimer.stop();

	}

	return run;
}

int Landscape::simulationFinish() {

	if(!bAborted) {
		//Clean exit:

		profiler->overallTimer.stop();

		//Have each intervention perform its final action
		interventionManager->finalInterventions();

		return 1;

	} else {
		//Called for abort:
		//TODO: Maybe dump critical things like RNG seed to a log file?
	}

	return 0;

}

int Landscape::debugCompareStateToDPCs() {
	//Check that DPCs are consistent with the state of the system:
	//iPop = nPopulations corresponds to Location level
	std::vector<Raster<int>> infectionRasters(PopulationCharacteristics::nPopulations + 1);

	std::vector<int> nPopulationsInfectedRaster(PopulationCharacteristics::nPopulations + 1);
	std::vector<int> nPopulationsInfectedDPC(PopulationCharacteristics::nPopulations + 1);

	for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations + 1; ++iPop) {
		infectionRasters[iPop].init(*getWorld()->header);
		infectionRasters[iPop].setAllRasterValues(0);

		nPopulationsInfectedDPC[iPop] = 0;
		nPopulationsInfectedRaster[iPop] = 0;
	}

	for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations; ++iPop) {

		//Raster state:
		for (auto iRow = 0; iRow < getWorld()->header->nRows; ++iRow) {
			for (auto iCol = 0; iCol < getWorld()->header->nCols; ++iCol) {

				double *dAlloc = getWorld()->locationArray[iCol + iRow*getWorld()->header->nCols]->getHostAllocations()[iPop];

				for (auto iClass = 0; iClass < Population::pGlobalPopStats[iPop]->nClasses; ++iClass) {
					if (iClass != Population::pGlobalPopStats[iPop]->iSusceptibleClass && iClass != Population::pGlobalPopStats[iPop]->iRemovedClass && dAlloc[iClass] > 0.0) {
						infectionRasters[iPop](iCol, iRow) = 1;
						infectionRasters[PopulationCharacteristics::nPopulations](iCol, iRow) = 1;
					}
				}

			}
		}

		//DPC State:
		nPopulationsInfectedDPC[iPop] = SummaryDataManager::getCurrentPopulationsInfected()[iPop];
	}
	nPopulationsInfectedDPC[PopulationCharacteristics::nPopulations] = SummaryDataManager::getCurrentLocationsInfected();


	int bAnyError = 0;
	for (int iPop = 0; iPop < PopulationCharacteristics::nPopulations + 1; ++iPop) {

		for (auto iRow = 0; iRow < getWorld()->header->nRows; ++iRow) {
			for (auto iCol = 0; iCol < getWorld()->header->nCols; ++iCol) {
				if (infectionRasters[iPop](iCol, iRow) > 0) {
					nPopulationsInfectedRaster[iPop] += 1;
				}
			}
		}

		if (nPopulationsInfectedDPC[iPop] != nPopulationsInfectedRaster[iPop]) {
			printf("ERROR: Run=%d, time=%.6f, DPC data does not match system state for ", getWorld()->runNo, getWorld()->time);
			if (iPop == PopulationCharacteristics::nPopulations) {
				printf("locations");
			} else {
				printf("population %d", iPop);
			}
			printf(" dpc: %d, state: %d", nPopulationsInfectedDPC[iPop], nPopulationsInfectedRaster[iPop]);
			printf("\n");
			bAnyError = 1;
		}
	}

	if (bAnyError) {
		return 0;
	}

	return 1;

}
