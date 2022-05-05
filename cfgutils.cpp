//cfgutils.cpp

#include "stdafx.h"
#include "Raster.h"
#include "PopulationCharacteristics.h"
#include "ColourRamp.h"
#include "InterventionManager.h"
#include "InterVideo.h"
#include "LocationMultiHost.h"
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <string.h>
#include <sstream>
#include "cfgutils.h"


#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

//Pointer to world for utility functions:
Landscape *pWorld;

char *szReadErrorLog;

const char *versionString;

template<> const char* OutFmt< int >::fmtStr = "%d";
template<> const char* OutFmt< bool >::fmtStr = "%d";
template<> const char* OutFmt< double >::fmtStr = "%11.9f";

template<> const char* OutFmt< int >::fmtImpreciseStr = "%d";
template<> const char* OutFmt< bool >::fmtImpreciseStr = "%d";
template<> const char* OutFmt< double >::fmtImpreciseStr = "%f";

int initCFG(Landscape *myWorld, const char *version) {

	pWorld = myWorld;
	pWorld->bReadingErrorFlag = 0;

	pWorld->szCurrentWorkingSection = "INITIALISING";
	pWorld->szCurrentWorkingSubsection = "";

	szReadErrorLog = new char[_N_MAX_DYNAMIC_BUFF_LEN];

	sprintf(szReadErrorLog, "ERROR LOG:\n");

	versionString = version;

	return 1;
}

Landscape *getWorld() {
	return pWorld;
}

const char *getVersionString() {
	return versionString;
}

int reportReadError(const char *fmt, ...){
	
	va_list argp;
	va_start(argp,fmt);

	char szTemp[_N_MAX_STATIC_BUFF_LEN];

	if(fmt != NULL) {
		vsprintf(szTemp, fmt, argp);
	}

	va_end(argp);
	
	sprintf(szReadErrorLog + strlen(szReadErrorLog), "%s", szTemp);
	printf("%s", szTemp);

	return pWorld->bReadingErrorFlag = 1;
}

char *getErrorMessages() {
	return szReadErrorLog;
}

int bIsKeyMode() {
	return pWorld->bGiveKeys;
}

char *szPrefixOutput() {
	return pWorld->szPrefixOutput;
}

int readValueToProperty(char *sKey, int* tProp, int required, char *valueString) {

	if(giveKeyInfo(sKey, tProp, required, valueString)) {
		return 1;
	}

	int tTemp;
	if(readValueFromCfg(pWorld->szMasterFileName, sKey, &tTemp)) {
		*tProp = tTemp;
		return 1;
	} else {
		if(required >= 1) {
			//Value is always required
			reportReadError("Error: Could not find required data: %s\n",sKey);

			return 0;
		} else {
			//Value is optional
			return 0;
		}
	}
}

int readValueToProperty(char *sKey, double* tProp, int required, char *valueString) {

	if(giveKeyInfo(sKey, tProp, required, valueString)) {
		return 1;
	}

	double tTemp;
	if(readValueFromCfg(pWorld->szMasterFileName, sKey, &tTemp)) {
		*tProp = tTemp;
		return 1;
	} else {
		if(required >= 1) {
			//Value is always required
			reportReadError("Error: Could not find required data: %s\n",sKey);

			return 0;
		} else {
			//Value is optional
			return 0;
		}
	}
}

int readValueToProperty(char *sKey, char* tProp, int required, char *valueString) {

	if(giveKeyInfo(sKey, tProp, required, valueString)) {
		return 1;
	}

	char tTemp[_N_MAX_DYNAMIC_BUFF_LEN];
	if(readValueFromCfg(pWorld->szMasterFileName, sKey, tTemp)) {
		sprintf(tProp,"%s", tTemp);
		return 1;
	} else {
		if(required >= 1) {
			//Value is always required
			reportReadError("Error: Could not find required data: %s\n",sKey);

			return 0;
		} else {
			//Value is optional
			return 0;
		}
	}
}

int readValueToProperty(char *sKey, unsigned long* tProp, int required, char *valueString) {

	if(giveKeyInfo(sKey, tProp, required, valueString)) {
		return 1;
	}

	unsigned long tTemp;
	if(readValueFromCfg(pWorld->szMasterFileName, sKey, &tTemp)) {
		*tProp = tTemp;
		return 1;
	} else {
		if(required >= 1) {
			//Value is always required
			reportReadError("Error: Could not find required data: %s\n",sKey);

			return 0;
		} else {
			//Value is optional
			return 0;
		}
	}
}

int giveKeyInfo(char *sKey, int* tProp, int required, char *valueString) {
	if(pWorld->bGiveKeys) {
		//give info on key:
		FILE *pKeyFile = fopen(pWorld->szKeyFileName,"a");
		if(pKeyFile) {
			int reqLevel = required;
			if(reqLevel<0) {
				reqLevel *= -1;
			}
			//Indent:
			for(int i=0; i<reqLevel; i++) {
				fprintf(pKeyFile," ");
			}
			//Key:
			fprintf(pKeyFile,"%s",sKey);
			//Flag optional:
			if(required <= 0) {
				fprintf(pKeyFile,"*");
			}
			//Default value:
			fprintf(pKeyFile," %d", *tProp);

			//Value String:
			if(valueString != NULL) {
				fprintf(pKeyFile," ~i%s~",valueString);
			}

			fprintf(pKeyFile,"\n");

			fclose(pKeyFile);

			*tProp = 1;
			return 1;
		}
	}
	return 0;
}

int giveKeyInfo(char *sKey, double* tProp, int required, char *valueString) {
	if(pWorld->bGiveKeys) {
		//give info on key:
		FILE *pKeyFile = fopen(pWorld->szKeyFileName,"a");
		if(pKeyFile) {
			int reqLevel = required;
			if(reqLevel<0) {
				reqLevel *= -1;
			}
			//Indent:
			for(int i=0; i<reqLevel; i++) {
				fprintf(pKeyFile," ");
			}
			//Key:
			fprintf(pKeyFile,"%s",sKey);
			//Flag optional:
			if(required <= 0) {
				fprintf(pKeyFile,"*");
			}
			//Default value:
			fprintf(pKeyFile," %f", *tProp);

			//Value String:
			if(valueString != NULL) {
				fprintf(pKeyFile," ~f%s~",valueString);
			}

			fprintf(pKeyFile,"\n");

			fclose(pKeyFile);

			*tProp = 1.0;
			return 1;
		}
	}
	return 0;
}

int giveKeyInfo(char *sKey, char* tProp, int required, char *valueString) {
	if(pWorld->bGiveKeys) {
		//give info on key:
		FILE *pKeyFile = fopen(pWorld->szKeyFileName,"a");
		if(pKeyFile) {
			int reqLevel = required;
			if(reqLevel<0) {
				reqLevel *= -1;
			}
			//Indent:
			for(int i=0; i<reqLevel; i++) {
				fprintf(pKeyFile," ");
			}
			//Key:
			fprintf(pKeyFile,"%s",sKey);
			//Flag optional:
			if(required <= 0) {
				fprintf(pKeyFile,"*");
			}
			//Default value:
			if(tProp != NULL) {
				fprintf(pKeyFile," %s", tProp);
				sprintf(tProp, "=DEFAULT=");
			} else {
				fprintf(pKeyFile," NULL");
			}

			//Value String:
			if(valueString != NULL) {
				fprintf(pKeyFile," ~s%s~",valueString);
			}

			fprintf(pKeyFile,"\n");

			fclose(pKeyFile);

			return 1;
		}
	}
	return 0;
}

int giveKeyInfo(char *sKey, unsigned long* tProp, int required, char *valueString) {
	if(pWorld->bGiveKeys) {
		//give info on key:
		FILE *pKeyFile = fopen(pWorld->szKeyFileName,"a");
		if(pKeyFile) {
			int reqLevel = required;
			if(reqLevel<0) {
				reqLevel *= -1;
			}
			//Indent:
			for(int i=0; i<reqLevel; i++) {
				fprintf(pKeyFile," ");
			}
			//Key:
			fprintf(pKeyFile,"%s",sKey);
			//Flag optional:
			if(required <= 0) {
				fprintf(pKeyFile,"*");
			}
			//Default Value:
			fprintf(pKeyFile," %lu", *tProp);

			//Value String:
			if(valueString != NULL) {
				fprintf(pKeyFile," ~l%s~",valueString);
			}

			fprintf(pKeyFile,"\n");

			fclose(pKeyFile);

			*tProp = 1;
			return 1;
		}
	}
	return 0;
}

int readValueFromCfg(const char *szCfgFile, char *szKey, char *tValue) {
	return readStringFromCfg(szCfgFile, szKey, tValue);
}

int readValueFromCfg(const char *szCfgFile, char *szKey, int *tValue) {
	return readIntFromCfg(szCfgFile, szKey, tValue);
}

int readValueFromCfg(const char *szCfgFile, char *szKey, double *tValue) {
	return readDoubleFromCfg(szCfgFile, szKey, tValue);
}

int readValueFromCfg(const char *szCfgFile, char *szKey, unsigned long *tValue) {
	return readUnsignedLongFromCfg(szCfgFile, szKey, tValue);
}

int setCurrentWorkingSection(const char* szSectionName)  {

	pWorld->szCurrentWorkingSection = szSectionName;

	giveKeyHeader(1);

	return 1;
}

int setCurrentWorkingSubection(const char* szSubsectionName, ConfigModes mode)  {

	pWorld->szCurrentWorkingSubsection = szSubsectionName;

	giveKeyHeader(0, mode);

	return 1;
}

int giveKeyHeader(int isSectionHeader, ConfigModes mode) {

	if(pWorld->bGiveKeys) {

		printToFileAndScreen(pWorld->szKeyFileName, 0, "\n\n");
		if(isSectionHeader) {
			printHeaderText(pWorld->szKeyFileName," %s ", pWorld->szCurrentWorkingSection);
		} else {
			printToFileAndScreen(pWorld->szKeyFileName, 0, "=%s=",pWorld->szCurrentWorkingSubsection);
			switch (mode)
			{
			case CM_SIM:
				printToFileAndScreen(pWorld->szKeyFileName, 0, " ~%s~","SIMULATION");
				break;
			case CM_MODEL:
				printToFileAndScreen(pWorld->szKeyFileName, 0, " ~%s~","MODEL");
				break;
			case CM_APPLICATION:
				printToFileAndScreen(pWorld->szKeyFileName, 0, " ~%s~","APPLICATION");
				break;
			case CM_POLICY:
				printToFileAndScreen(pWorld->szKeyFileName, 0, " ~%s~","POLICY");
				break;
			case CM_ENSEMBLE:
				printToFileAndScreen(pWorld->szKeyFileName, 0, " ~%s~","ENSEMBLE");
				break;
			default:
				printToFileAndScreen(pWorld->szKeyFileName, 0, " ~%s~","ERROR");
				break;
			}
			printToFileAndScreen(pWorld->szKeyFileName, 0, "\n");
		}

	}

	return 1;
}

int clock_ms() {
	return clock()/TIMEFACTOR;
}


/* work out configuration file name and check whether it exists */
int	getCfgFileName(char *szProgName, char *szCfgFile) {
	char	*pPtr;
	FILE 	*fp;

	szCfgFile[0] = '\0';

	if((pPtr = strrchr(szProgName,C_DIR_DELIMITER))!=NULL) {
		strcpy(szCfgFile,pPtr+1);
	} else {
		strcpy(szCfgFile,szProgName);
	}
	if((pPtr = strstr(szCfgFile,".exe"))!=NULL) {
		*pPtr = '\0';
	}
	strcat(szCfgFile,".cfg");
	/* check file exists */
	fp = fopen(szCfgFile, "rb");
	if(fp) {
		fclose(fp);
		return 1;
	}
	return 0;
}

/* (slow) routines to find values from a cfg file */
int findKey(const char *szCfgFile, char *szKey, char *szValue) {
	
	int	 nFound = 0;

	FILE *fp = fopen(szCfgFile,"rb");
	if(fp) {
		/*Convert line and key to upper case to make searching case insensitive*/
		char szLine[_N_MAX_DYNAMIC_BUFF_LEN];
		char szUpperLine[_N_MAX_DYNAMIC_BUFF_LEN];
		char szUpperKey[_N_MAX_DYNAMIC_BUFF_LEN];

		sprintf(szUpperKey,"%s", szKey);
		makeUpper(szUpperKey, 0);

		while(fgets(szLine, _N_MAX_DYNAMIC_BUFF_LEN,fp)) {
			char *pPtr;
			sprintf(szUpperLine, "%s", szLine);
			makeUpper(szUpperLine, 0);

			char *pLine = szUpperLine;

			//Strip off any whitespace at the start:
			size_t nWS = strspn(pLine, " \t");
			pLine += nWS;

			//Terminate the line at the first occurrence of whitespace (or newline if there isn't any...):
			if ((pPtr = strpbrk(pLine, " \t\r\n")) != NULL) {
				*pPtr = '\0';
			}

			if (strcmp(szUpperKey, pLine) == 0) {//Test against uppercase

				nFound++;//We have found a match for the key

				pPtr = strpbrk(szLine + nWS, " \t");//Find where original (non-uppercased) value portion of string starts
				if (pPtr != NULL) {
					//The key did come with a value part:
					strcpy(szValue, pPtr + 1);
					/* strip off newline (if any) */
					if ((pPtr = strpbrk(szValue, "\r\n")) != NULL) {
						*pPtr = '\0';
					}
				} else {
					//If the key didn't have a value part attached, we return the empty string
					sprintf(szValue, "%s", "");
				}

			}

		}

		fclose(fp);
	}

	if (nFound > 1) {
		std::string exceptMsg = std::string("ERROR: Found multiple occurrences of key \"") + std::string(szKey) + std::string("\" in config file \"") + std::string(szCfgFile) + std::string("\"\n");
		throw std::runtime_error(exceptMsg.c_str());
	}

	return (nFound == 1);
}

int readStringFromCfg(const char *szCfgFile, char *szKey, char *szValue) {
	return(findKey(szCfgFile,szKey,szValue));
}

int readDoubleFromCfg(const char *szCfgFile, char *szKey, double *pdValue) {

	char szValue[N_MAXSTRLEN];

	if(findKey(szCfgFile,szKey,szValue)) {
		try {
			*pdValue = stod(szValue);
		}
		catch (std::exception &e) {
			std::string exceptMsg = std::string("ERROR: Unable to parse value \"") +std::string(szValue) + std::string("\" (") + std::string(szKey) + std::string(") to double\n");
			throw std::runtime_error(exceptMsg.c_str());
		}
		return 1;
	}
	return 0;
}

int readIntFromCfg(const char *szCfgFile, char *szKey, int *pnValue) {
	char szValue[N_MAXSTRLEN];

	if(findKey(szCfgFile,szKey,szValue)) {
		try {
			*pnValue = stoi(szValue);
		}
		catch (std::exception &e) {
			std::string exceptMsg = std::string("ERROR: Unable to parse value \"") + std::string(szValue) + std::string("\" (") + std::string(szKey) + std::string(") to int\n");
			throw std::runtime_error(exceptMsg.c_str());
		}
		return 1;
	}
	return 0;
}

int readUnsignedLongFromCfg(const char *szCfgFile, char *szKey, unsigned long *pnValue) {
	char szValue[N_MAXSTRLEN];

	if(findKey(szCfgFile,szKey,szValue)) {
		try {
			*pnValue = stoul(szValue, NULL, 10);
		}
		catch (std::exception &e) {
			std::string exceptMsg = std::string("ERROR: Unable to parse value \"") + std::string(szValue) + std::string("\" (") + std::string(szKey) + std::string(") to unsigned long\n");
			throw std::runtime_error(exceptMsg.c_str());
		}
		return 1;
	}
	return 0;
}

int printToKeyFile(const char *fmt, ...) {
	va_list argp;
	va_start(argp,fmt);

	if(pWorld->bGiveKeys) {
		FILE *pKeyFile = fopen(pWorld->szKeyFileName, "a");
		if(pKeyFile) {
			vfprintf(pKeyFile, fmt, argp);
			fclose(pKeyFile);
		} else {
			return 1;
		}
	}

	va_end(argp);

	return 0;
}

int readRasterToKernel(const char *szCfgFile, int nK, double *pArray)
{/*Different to general reading files as reads kernel*/
	FILE 	*fIn;
	char	*szBuff,*pPtr;
	int		bRet,x,y,i;
	double	dVal;

	bRet = 0;
	fIn = fopen(szCfgFile,"rb");
	if(fIn) {
		szBuff = (char*)malloc(_N_MAX_DYNAMIC_BUFF_LEN*sizeof(char));
		if(szBuff) {
			i = 0;
			x = 0;
			y = 0; 
			bRet = 1;
			//strip away header
			if (!fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
				fprintf(stderr, "raster reading: could not read header line\n");
				bRet = 0;
			}
			/* will fail on long lines */
			while(bRet && fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
				if(y >= 1+2*nK) {
					if(bRet) {
						fprintf(stderr, "raster reading: y too large\n");
						printf("x=%d, y=%d\n",x,y);
						bRet = 0;
					}
				}
				pPtr = strpbrk(szBuff, "\r\n");
				if(pPtr) {
					pPtr[0] = '\0';
				} else {
					fprintf(stderr, "raster reading: line too long\n");
					printf("x=%d, y=%d\n",x,y);
					bRet=0;				/* no CR or NL means line not read in fully */
				}
				pPtr = strtok(szBuff, " \t");
				while(bRet && pPtr) {
					dVal = atof(pPtr);
					if(x < 1+2*nK) {
						//take in value
						pArray[x+(1+2*nK)*y] = dVal;
					} else {
						bRet = 0;
						fprintf(stderr, "raster reading: x too large\n"); /* too large in x direction */
						printf("x=%d, y=%d\n",x,y);
					}
					i++;
					x++;
					pPtr = strtok(NULL, " \t");
				}
				if(x==1+2*nK) {
					x=0;
					y++;
				}
			}
			if(y!=1+2*nK) {
				if(bRet) {
					fprintf(stderr, "raster reading: y too small\n");
					printf("x=%d, y=%d\n",x,y);
					bRet = 0;
				}
			} else {
				if(bRet && i == (1+2*nK)*(1+2*nK)) {
					;
					/* all worked */
				} else {
					if(bRet) {
						fprintf(stderr, "raster reading: not read in enough points\n");
						printf("x=%d, y=%d\n",x,y);
						bRet = 0;
					}
				}
			}
			free(szBuff);
		}
		fclose(fIn);
	}

	return bRet;
	
}

int writeRasterFileHeader(const char *szFileName, double NODATA) {
	return writeRasterFileHeaderRestricted(szFileName, 0, pWorld->header->nCols - 1, 0, pWorld->header->nRows - 1, NODATA);
}

int writeRasterFileHeaderRestricted(const char *szFileName, int xMin, int xMax, int yMin, int yMax, double NODATA) {

	FILE *pRASFILE;
	int maxW = pWorld->header->nCols;
	int maxH = pWorld->header->nRows;

	int actualW = xMax-xMin+1;
	int actualH = yMax-yMin+1;
	double actualxll = pWorld->header->xllCorner + pWorld->header->cellsize*xMin;
	double actualyll = pWorld->header->yllCorner - pWorld->header->cellsize*(yMax-(pWorld->header->nRows-1));

	pRASFILE = fopen(szFileName,"w");
	if(pRASFILE) {
		//Write GIS header
		fprintf(pRASFILE,"ncols         %d\n",actualW);
		fprintf(pRASFILE,"nrows         %d\n",actualH);
		fprintf(pRASFILE,"xllcorner       %f\n",actualxll);
		fprintf(pRASFILE,"yllcorner       %f\n",actualyll);
		fprintf(pRASFILE,"cellsize      %11.11f\n",pWorld->header->cellsize);
		fprintf(pRASFILE,"NODATA_value     %d\n",pWorld->header->NODATA);

		fclose(pRASFILE);

	} else {
		printf("ERROR: Unable to open file %s for writing\n",szFileName);
		return 0;
	}

	return 1;
}
	
int writeRasterHostAllocation(const char *szFileName, int iPop, int iClass) {

	FILE *pRASFILE;
	int maxW = pWorld->header->nCols;
	int maxH = pWorld->header->nRows;
	char *buff = new char[_N_MAX_DYNAMIC_BUFF_LEN];
	char *ptr = buff;

	double *dAlloc;

	//Write GIS header
	writeRasterFileHeader(szFileName);

	pRASFILE = fopen(szFileName,"a");
	//write GIS Raster:
	for(int j=0;j<maxH;j++) {
		for(int i=0;i<maxW;i++) {
			if(pWorld->locationArray[i+j*maxW] != NULL && pWorld->locationArray[i+j*maxW]->getHostAllocations()[iPop] != NULL) {
				dAlloc = pWorld->locationArray[i+j*maxW]->getHostAllocations()[iPop];
				ptr += sprintf(ptr,"%11.9f ", dAlloc[iClass]);
			} else {
				ptr += sprintf(ptr,"%d ",pWorld->header->NODATA);
			}
		}
		sprintf(ptr,"\n");
		fprintf(pRASFILE,"%s", buff);
		ptr = buff;
	}

	fclose(pRASFILE);

	delete [] buff;

	return 1;
}

int writeRasterEpidemiologicalMetric(const char *szFileName, int iPop, int iMetric) {

	FILE *pRASFILE;
	int maxW = pWorld->header->nCols;
	int maxH = pWorld->header->nRows;
	char *buff = new char[_N_MAX_DYNAMIC_BUFF_LEN];
	char *ptr = buff;

	//Write GIS header
	double noData = -1.0;
	if (
		iMetric == PopulationCharacteristics::timeFirstInfection || 
		iMetric == PopulationCharacteristics::timeFirstRemoved || 
		iMetric == PopulationCharacteristics::timeFirstSymptomatic
		) {
		noData = min(-1.0, pWorld->timeStart - 1.0);//TODO: Coordinate magic nodata values
	}

	writeRasterFileHeader(szFileName, noData);


	pRASFILE = fopen(szFileName,"a");
	//write GIS Raster:
	for(int j=0;j<maxH;j++) {
		for(int i=0;i<maxW;i++) {
			double val = noData;
			if(pWorld->locationArray[i+j*maxW] != NULL) {
				val = pWorld->locationArray[i + j*maxW]->getEpidemiologicalMetric(iPop, iMetric);
			}
			ptr += sprintf(ptr, "%11.9f ", val);
		}
		sprintf(ptr,"\n");
		fprintf(pRASFILE,"%s", buff);
		ptr = buff;
	}

	fclose(pRASFILE);

	delete [] buff;

	return 1;
}

int readTimeStateValues(const char *fileName, int &nStates, vector<double> &pdTimes, vector<double> &pdValues) {
	ifstream inFile;
	string sLine;
	stringstream ssLine;

	inFile.open(fileName);

	if(inFile.is_open()) {

		//Discard header:
		getline(inFile, sLine);

		ssLine.clear();
		ssLine.str("");
		ssLine << sLine;

		ssLine >> sLine;//Dump "States"
		ssLine >> nStates;//Read No of States

		if(nStates <= 0) {
			reportReadError("ERROR: File %s found %d states (<=0)\n", fileName, nStates);
			return 0;
		}

		pdTimes.resize(nStates);
		pdValues.resize(nStates);

		int nCurrentState = 0;

		while(inFile.good() && nCurrentState < nStates) {

			getline(inFile, sLine);

			ssLine.clear();
			ssLine.str("");
			ssLine << sLine;
			if(ssLine >> pdTimes[nCurrentState]) {

				ssLine >> pdValues[nCurrentState];
				
				nCurrentState++;
			}

			//TODO: Put in a check to see if there are any more lines once all necessary ones are found...

		}
		inFile.close();

		//cout << "Expected " << nStates << " found " << nCurrentState;
		if(nCurrentState == nStates) {
			//cout << ", great!" << endl;
		} else {
			reportReadError("ERROR: File %s expected %d found %d states\n", fileName, nStates, nCurrentState);
			return 0;
			//cout << ", broken!" << endl;
		}

	}

	return 1;
}

int readTimeStateIDPairsToUniqueMap(const char *fileName, int &nStates, vector<double> &pdTimes, vector<string> &pvStateIds, map<string, int> &pIdToIndexMapping) {

	ifstream inFile;
	string sLine;
	stringstream ssLine;

	inFile.open(fileName);

	if(inFile.is_open()) {

		//Discard header:
		getline(inFile, sLine);

		ssLine.clear();
		ssLine.str("");
		ssLine << sLine;

		ssLine >> sLine;//Dump "nStates"
		ssLine >> nStates;//Read No of States

		if(nStates <= 0) {
			reportReadError("ERROR: File %s found %d states (<=0)\n", fileName, nStates);
			return 0;
		}

		pdTimes.resize(nStates);


		int nCurrentState = 0;

		while(inFile.good() && nCurrentState < nStates) {

			getline(inFile, sLine);

			ssLine.clear();
			ssLine.str("");
			ssLine << sLine;
			if(ssLine >> pdTimes[nCurrentState]) {

				string sID;
				ssLine >> sID;
				//stringToUpper(&sID);
				pvStateIds.push_back(sID);

				nCurrentState++;
			}

			//TODO: Put in a check to see if there are any more lines once all necessary ones are found...

		}
		inFile.close();

		//cout << "Expected " << nStates << " found " << nCurrentState;
		if(nCurrentState == nStates) {
			//cout << ", great!" << endl;
		} else {
			reportReadError("ERROR: File %s expected %d found %d states\n", fileName, nStates, nCurrentState);
			return 0;
			//cout << ", broken!" << endl;
		}

		//Store list of all the different identifiers and the corresponding indices:
		int nIDs = 0;

		for(size_t iState=0; iState < pvStateIds.size(); iState++) {
			//If ID not already entered:
			if(pIdToIndexMapping.find(pvStateIds[iState]) == pIdToIndexMapping.end()) {
				//add ID to mapping:
				pIdToIndexMapping[pvStateIds[iState]] = nIDs;
				nIDs++;
			}
		}

	} else {
		return 0;
	}

	return 1;
}

int checkPresent(const char *szCfgFile) {
	int	 bRet;
	FILE *fp;

	bRet = 0;
	fp = fopen(szCfgFile,"rb");
	if(fp) {
		bRet = 1;
		fclose(fp);
	}
	return bRet;
}

int makeUpper(char *sPtr, int stripTrailingWhiteSpace) {

	//make it upper case
	while(*sPtr != '\0') {
		*sPtr = toupper((unsigned char) *sPtr);
		sPtr++;
	}

	if(stripTrailingWhiteSpace) {
		//strip any spaces off the end
		while(!isalpha(*sPtr)) {
			*sPtr = '\0';
			sPtr--;
		}
	}

	return 1;
}


void stringToUpper(std::string &strToConvert) {
	for (auto &c : strToConvert) {
		c = toupper(c);
	}
	return;
}

std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

int compare (const void * a, const void * b) {
	return ( *(int*)a - *(int*)b );
}

int	printHeaderText(const char *fName, const char *fmt, ...) {

	char textBuffer[1024];
	char textBuffer2[1024];
	char *buff;

	va_list argp;
	va_start(argp,fmt);
	vsprintf(textBuffer, fmt, argp);
	va_end(argp);

	size_t l = strlen(textBuffer);
	buff = textBuffer2;
	for(size_t i=0; i<l; i++) {
		buff += sprintf(buff,"=");
	}
	sprintf(buff,"\n");

	if(fName != NULL) {
		FILE *pF;
		pF = fopen(fName,"a");
		if(pF) {
			fprintf(pF,"\n%s",textBuffer2);
			fprintf(pF,"%s\n",textBuffer);
			fprintf(pF,"%s\n",textBuffer2);
			fclose(pF);
		} else {
			//Error: Unable to write to file!;
		}
	} else {
		printf("\n%s",textBuffer2);
		printf("%s\n",textBuffer);
		printf("%s\n",textBuffer2);
	}

	return 0;
}

int	printHeaderText(FILE *pF, const char *fmt, ...) {

	char textBuffer[1024];
	char textBuffer2[1024];
	char *buff;

	va_list argp;
	va_start(argp,fmt);
	vsprintf(textBuffer, fmt, argp);
	va_end(argp);

	size_t l = strlen(textBuffer);
	buff = textBuffer2;
	for(size_t i=0; i<l; i++) {
		buff += sprintf(buff,"=");
	}
	sprintf(buff,"\n");

	if(pF) {
		fprintf(pF,"\n%s",textBuffer2);
		fprintf(pF,"%s\n",textBuffer);
		fprintf(pF,"%s\n",textBuffer2);
	} else {
		printf("\n%s",textBuffer2);
		printf("%s\n",textBuffer);
		printf("%s\n",textBuffer2);
	}

	return 0;
}

int printToFileAndScreen(const char *fName, const int toScreen, const char *fmt, ...) {

	va_list argp;

	//Print to screen:
	if(toScreen) {
		va_start(argp,fmt);
		vprintf(fmt, argp);
		va_end(argp);
	}

	//Print to file
	if(fName != NULL) {
		FILE *pF;
		pF = fopen(fName,"a");

		if(pF) {

			va_start(argp,fmt);
			vfprintf(pF,fmt, argp);
			va_end(argp);

			fclose(pF);

		} else {
			//Error: Unable to write to file!;
		}
	}

	return 0;
}

int printToFilePointerAndScreen(FILE *pFile, const int toScreen, const char *fmt, ...) {

	va_list argp;

	//Print to screen:
	if(toScreen) {
		va_start(argp,fmt);
		vprintf(fmt, argp);
		va_end(argp);
	}

	//Print to file
	if(pFile != NULL) {
		va_start(argp,fmt);
		vfprintf(pFile,fmt, argp);
		va_end(argp);
	}

	return 0;
}

int printC(const int toScreen, const char *fmt, ...) {

	va_list argp;
	va_start(argp,fmt);

	//Print to screen:
	if(toScreen) {
		vprintf(fmt, argp);
	}

	va_end(argp);

	return 0;
}

int printk(const char *fmt, ...) {
	va_list argp;
	va_start(argp,fmt);

	//Print to screen:
	if(!pWorld->bGiveKeys) {
		vprintf(fmt, argp);
	}

	va_end(argp);

	return 0;
}

int writeNewFile(const char *fName) {
	FILE *pFile;
	pFile = fopen(fName,"w");
	if(pFile) {
		fclose(pFile);
		return 0;
	} else {
		return 1;
	}
}

//template <class T>
int writeRasterToFile(const char *title, Raster<int> *data, ColourRamp *ramp) {
#if USE_VIDEO
	pWorld->interventionManager->pInterVideo->writeRasterToFile(title, data, ramp);
#else
	printf("Graphical output is not enabled\n");
#endif
	return 1;
}

void no_storage() {
	std::cerr << "\n\nFatal Error: Out of memory.\n\n";
#ifdef _WIN32
	std::exit(2);
#else
	exit(2);
#endif
}
