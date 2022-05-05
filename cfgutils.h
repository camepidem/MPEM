#ifndef _CFGUTILS_H_
#define _CFGUTILS_H_ 1

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

#include "classes.h"
#include <vector>
#include <string>
#include <map>
#include "Landscape.h"


#define		N_MAXFNAMELEN			256				/* Maximum length of a filename */
#define		N_MAXSTRLEN				1024			/* Maximum length of a string */
#define		N_LINEBLOCKSIZE			256				/* How many lines to allocate at once */
#define		N_MAXREADINLEN			8192			/* Max length of input line */
#define		_N_MAX_STATIC_BUFF_LEN		1024
#define		_N_MAX_DYNAMIC_BUFF_LEN		128*_N_MAX_STATIC_BUFF_LEN	/* trying to put this much on the stack all at once crashes some compilers */

/***********************/
/* platform specific   */
/***********************/
#ifdef _WIN32
#define 	C_DIR_DELIMITER '\\'
#else
#define 	C_DIR_DELIMITER '/'
#endif

//Timing:
#ifdef _WIN32
#define 	TIMEFACTOR 1
#else
#define 	TIMEFACTOR 1000
#endif

//Pointer to landscape
int		initCFG(Landscape *myWorld, const char *version);

Landscape * getWorld();

const char* getVersionString();

int		clock_ms();

//Data reading methods:
int		reportReadError(const char *fmt, ...);
char	*getErrorMessages();


int		bIsKeyMode();

char	*szPrefixOutput();

int		readValueToProperty(char *sKey, int* tProp, int required=1, char *valueString=NULL);
int		readValueToProperty(char *sKey, double* tProp, int required=1, char *valueString=NULL);
int		readValueToProperty(char *sKey, char* tProp, int required=1, char *valueString=NULL);
int		readValueToProperty(char *sKey, unsigned long *tProp, int required=1, char *valueString=NULL);

int 	readValueFromCfg(const char *szCfgFile, char *szKey, int *tValue);
int 	readValueFromCfg(const char *szCfgFile, char *szKey, double *tValue);
int 	readValueFromCfg(const char *szCfgFile, char *szKey, char *tValue);
int 	readValueFromCfg(const char *szCfgFile, char *szKey, unsigned long *tValue);

int		setCurrentWorkingSection(const char* szSectionName);
enum ConfigModes
{
	CM_SIM, CM_MODEL, CM_APPLICATION, CM_POLICY, CM_ENSEMBLE, CM_MAX
};
int		setCurrentWorkingSubection(const char* szSubsectionName, ConfigModes mode=CM_SIM);

int		giveKeyHeader(int isFirst=0, ConfigModes mode=CM_SIM);

int		giveKeyInfo(char *sKey, int* tProp, int required, char *valueString=NULL);
int		giveKeyInfo(char *sKey, double* tProp, int required, char *valueString=NULL);
int		giveKeyInfo(char *sKey, char* tProp, int required, char *valueString=NULL);
int		giveKeyInfo(char *sKey, unsigned long *tProp, int required, char *valueString=NULL);

int		getCfgFileName(const char *szProgName, char *szCfgFile);
int		findKey(const char *szCfgFile, char *szKey, char *szValue);
int 	readStringFromCfg(const char *szCfgFile, char *szKey, char *szValue);
int 	readDoubleFromCfg(const char *szCfgFile, char *szKey, double *pdValue);
int 	readIntFromCfg(const char *szCfgFile, char *szKey, int *pnValue);
int 	readUnsignedLongFromCfg(const char *szCfgFile, char *szKey,unsigned long *pnValue);

//Horrors of include loops pop up in g++ because of templates requiring code to be in the headers...
#include "RasterHeader.h"

int		printToKeyFile(const char *fmt, ...);

//Raster Reading
//To Locations:
int		readRasterToKernel(const char *szCfgFile, int nK, double *pArray);

template<typename T>
int		readRasterToProperty(const char *szFileName, int iPop, int (LocationMultiHost::*fnPtr)(T, int));

//To Data Structures:
template<typename T>
int		readRasterToArray(const char *szFileName, T *Array);

template<typename T>
int		readRasterToArray(const char *szFileName, T *Array, int nRows, int nCols);
template <typename T>
int		readRasterToVector(std::string fromFileName, std::vector<T> &Array, int nRows, int nCols);

//Raster Writing:
int		writeRasterFileHeader(const char *szFileName, double NODATA = -1.0);
int		writeRasterFileHeaderRestricted(const char *szFileName, int xMin, int xMax, int yMin, int yMax, double NODATA = -1.0);

//From Locations:
int		writeRasterHostAllocation(const char *szFileName, int iPop, int iClass);
int		writeRasterEpidemiologicalMetric(const char *szFileName, int iPop, int iMetric);

template< typename T >
class OutFmt {
private:
	static const char* fmtStr;
	static const char* fmtImpreciseStr;
public:
	static const char *fmt() { return fmtStr; };
	static const char *fmtType() { return fmtImpreciseStr; };
};

template <typename T>
int		writeRasterPopulationProperty(const char *szFileName, int iPop, T (LocationMultiHost::*fnPtr)(int));

//From Data Structures:
template <typename T>
int		writeRaster(const char *szCfgFile, T *rasterArray);

template <typename T>
int		writeRasterRestricted(const char *szCfgFile, Landscape *world, T *rasterArray, int xMin, int xMax, int yMin, int yMax);

//TODO: These are not very pretty, one day use a proper parsing library to eliminate them:
template <typename T>
int		readTable(const char *szFileName, int length, int nSkip, T *col1);

template <typename T, typename U>
int		readTable(const char *szFileName, int length, int nSkip, T *col1, U* col2);

template <typename T, typename U, typename V>
int		readTable(const char *szFileName, int length, int nSkip, T *col1, U* col2, V* col3);

template<typename T>
int		writeTable(const char * szFileName, int length, int append, T *col1);

template<typename T, typename U>
int		writeTable(const char * szFileName, int length, int append, T *col1, U* col2);

template<typename T, typename U, typename V>
int		writeTable(const char * szFileName, int length, int append, T *col1, U* col2, V* col3);

int		readTimeStateValues(const char *fileName, int &nStates, std::vector<double> &pdTimes, std::vector<double> &pdValues);
int		readTimeStateIDPairsToUniqueMap(const char *fileName, int &nStates, std::vector<double> &pdTimes, std::vector<std::string> &pvStateIds, std::map<std::string, int> &pIdToIndexMapping);

int		checkPresent(const char *szCfgFile);

int		makeUpper(char *sPtr, int stripTrailingWhiteSpace=1);
void	stringToUpper(std::string &strToConvert);

struct splitTypes
{
  enum empties_t { empties_ok, no_empties };
};

template <typename Container>
Container& split(
  Container&                            result,
  const typename Container::value_type& s,
  const typename Container::value_type& delimiters,
  splitTypes::empties_t                 empties = splitTypes::empties_ok )
{
  result.clear();
  size_t current;
  size_t next = -1;
  do
  {
    if (empties == splitTypes::no_empties)
    {
      next = s.find_first_not_of( delimiters, next + 1 );
      if (next == Container::value_type::npos) break;
      next -= 1;
    }
    current = next + 1;
    next = s.find_first_of( delimiters, current );
    result.push_back( s.substr( current, next - current ) );
  }
  while (next != Container::value_type::npos);
  return result;
}

// trim from start
std::string &ltrim(std::string &s);
// trim from end
std::string &rtrim(std::string &s);
// trim from both ends
std::string &trim(std::string &s);

int		compare (const void * a, const void * b);

int		printHeaderText(const char *fName, const char *fmt, ...);

int		printHeaderText(FILE *pF, const char *fmt, ...);

int		printToFileAndScreen(const char *fName, const int toScreen, const char *fmt, ...);
int		printToFilePointerAndScreen(FILE *pFile, const int toScreen, const char *fmt, ...);

int		printC(const int toScreen, const char *fmt, ...);
int		printk(const char *fmt, ...);

int		writeNewFile(const char *fName);

int		writeRasterToFile(const char *title, Raster<int> *data, ColourRamp *ramp);

//new 'new' behaviour
void no_storage();

template <typename T>
int readRasterToArray(const char *szCfgFile, T *Array, int nRows, int nCols) {
	FILE 	*fIn;
	char	*szBuff, *pPtr;
	int		bRet, x, y, i;

	bRet = 0;
	fIn = fopen(szCfgFile, "rb");
	if (fIn) {
		szBuff = (char*)malloc(_N_MAX_DYNAMIC_BUFF_LEN * sizeof(char));
		if (szBuff) {
			i = 0;
			x = 0;
			y = 0;
			bRet = 1;
			//strip away header
			for (int iHeaderLine = 0; iHeaderLine < 6; ++iHeaderLine) {
				auto x = fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn);
			}
			/* will fail on long lines */
			while (bRet && fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
				if (y >= nRows) {
					if (bRet) {
						fprintf(stderr, "raster reading: y too large\n");
						printf("x=%d, y=%d\n", x, y);
						bRet = 0;
					}
				}
				pPtr = strpbrk(szBuff, "\r\n");
				if (pPtr) {
					pPtr[0] = '\0';
				} else {
					fprintf(stderr, "raster reading: line too long\n");
					printf("x=%d, y=%d\n", x, y);
					bRet = 0;				/* no CR or NL means line not read in fully */
				}
				pPtr = strtok(szBuff, " \t");
				while (bRet && pPtr) {
					T TVal;
					std::stringstream ss;
					ss << std::string(pPtr);
					if (ss >> TVal) {
						//Parsing ok
						if (x < nCols) {
							//Check if value is possible, and should be set
							Array[x + y*nCols] = TVal;
						} else {
							bRet = 0;
							fprintf(stderr, "raster reading: x too large\n"); /* too large in x direction */
							printf("x=%d, y=%d\n", x, y);
						}
					} else {
						bRet = 0;
						reportReadError("ERROR: Unable to parse value of raster %s at x=%d, y=%d\n", szCfgFile, x, y);
					}
					i++;
					x++;
					pPtr = strtok(NULL, " \t");
				}
				if (x == nCols) {
					x = 0;
					y++;
				}
			}
			if (y != nRows) {
				if (bRet) {
					fprintf(stderr, "raster reading: y too small\n");
					printf("x=%d, y=%d\n", x, y);
					bRet = 0;
				}
			} else {
				if (bRet && i == nCols*nRows) {
					;
					/* all worked */
				} else {
					if (bRet) {
						fprintf(stderr, "raster reading: not read in enough points\n");
						printf("x=%d, y=%d\n", x, y);
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

template <typename T>
int readRasterToVector(std::string fromFileName, std::vector<T> &Array, int nRows, int nCols) {
	return readRasterToArray(fromFileName.c_str(), &Array[0], nRows, nCols);
}

template <typename T>
int readRasterToArray(const char *szFileName, T *Array) {
	Landscape *pWorld = getWorld();
	return readRasterToArray(szFileName, &Array[0], pWorld->header->nRows, pWorld->header->nCols);
}

template <typename T>
int readRasterToProperty(const char *szFileName, int iPop, int (LocationMultiHost::*fnPtr)(T, int)) {
	FILE 	*fIn;
	char	*szBuff, *pPtr;
	int		bRet, x, y, i;

	Landscape *pWorld = getWorld();

	bRet = 0;
	fIn = fopen(szFileName, "rb");
	if (fIn) {
		szBuff = (char*)malloc(_N_MAX_DYNAMIC_BUFF_LEN * sizeof(char));
		if (szBuff) {
			i = 0;
			x = 0;
			y = 0;
			bRet = 1;
			//strip away header
			for (int iHeaderLine = 0; iHeaderLine < 6; ++iHeaderLine) {
				auto x = fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn);
			}
			/* will fail on long lines */
			while (bRet && fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
				if (y >= pWorld->header->nRows) {
					if (bRet) {
						fprintf(stderr, "raster reading: y too large\n");
						printf("x=%d, y=%d\n", x, y);
						bRet = 0;
					}
				}
				pPtr = strpbrk(szBuff, "\r\n");
				if (pPtr) {
					pPtr[0] = '\0';
				} else {
					fprintf(stderr, "raster reading: line too long\n");
					printf("x=%d, y=%d\n", x, y);
					bRet = 0;				/* no CR or NL means line not read in fully */
				}
				pPtr = strtok(szBuff, " \t");
				while (bRet && pPtr) {
					T TVal;
					std::stringstream ss;
					ss << std::string(pPtr);
					if (ss >> TVal) {
						if (x < pWorld->header->nCols) {
							//Check if value is possible, and should be set
							//more error checking later, speed data reading process by not doing too much checking now...
							if (pWorld->locationArray[x + y*pWorld->header->nCols] != NULL) {
								//Here value is actually set
								(pWorld->locationArray[x + y*pWorld->header->nCols]->*fnPtr)(TVal, iPop);
							}
						} else {
							bRet = 0;
							fprintf(stderr, "raster reading: x too large\n"); /* too large in x direction */
							printf("x=%d, y=%d\n", x, y);
						}
					} else {
						bRet = 0;
						reportReadError("ERROR: Unable to parse value of raster %s at x=%d, y=%d\n", szFileName, x, y);
					}
					i++;
					x++;
					pPtr = strtok(NULL, " \t");
				}
				if (x == pWorld->header->nCols) {
					x = 0;
					y++;
				}
			}
			if (y != pWorld->header->nRows) {
				if (bRet) {
					fprintf(stderr, "raster reading: y too small\n");
					printf("x=%d, y=%d\n", x, y);
					bRet = 0;
				}
			} else {
				if (bRet && i == pWorld->header->nCols*pWorld->header->nRows) {
					;
					/* all worked */
				} else {
					if (bRet) {
						fprintf(stderr, "raster reading: not read in enough points\n");
						printf("x=%d, y=%d\n", x, y);
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

template <typename T>
int writeRasterPopulationProperty(const char *szFileName, int iPop, T(LocationMultiHost::*fnPtr)(int)) {

	Landscape *pWorld = getWorld();

	FILE *pRASFILE;
	int maxW = pWorld->header->nCols;
	int maxH = pWorld->header->nRows;
	char *buff = new char[_N_MAX_DYNAMIC_BUFF_LEN];
	char *ptr = buff;

	//Write GIS header
	writeRasterFileHeader(szFileName);

	char outFmtSpecifier[64];
	sprintf(outFmtSpecifier, "%s ", OutFmt<T>::fmt());

	pRASFILE = fopen(szFileName, "a");
	//write GIS Raster:
	for (int j = 0; j<maxH; j++) {
		for (int i = 0; i<maxW; i++) {
			if (pWorld->locationArray[i + j*maxW] != NULL) {
				ptr += sprintf(ptr, outFmtSpecifier, (pWorld->locationArray[i + j*maxW]->*fnPtr)(iPop));
			} else {
				ptr += sprintf(ptr, "%d ", pWorld->header->NODATA);
			}
		}
		sprintf(ptr, "\n");
		fprintf(pRASFILE, "%s", buff);
		ptr = buff;
	}

	fclose(pRASFILE);

	delete[] buff;

	return 1;

}

template <typename T>
int writeRaster(const char *szCfgFile, T *rasterArray) {
	Landscape *pWorld = getWorld();
	return writeRasterRestricted(szCfgFile, pWorld, rasterArray, 0, pWorld->header->nCols - 1, 0, pWorld->header->nRows - 1);
}

template <typename T>
int writeRasterRestricted(const char *szCfgFile, Landscape *world, T *rasterArray, int xMin, int xMax, int yMin, int yMax) {
	FILE *pRASFILE;
	int maxW = world->header->nCols;
	int maxH = world->header->nRows;
	char *buff = new char[_N_MAX_DYNAMIC_BUFF_LEN];
	char *ptr = buff;

	char outFmtSpecifier[64];
	sprintf(outFmtSpecifier, "%s ", OutFmt<T>::fmt());

	//Write GIS header
	writeRasterFileHeaderRestricted(szCfgFile, xMin, xMax, yMin, yMax);

	pRASFILE = fopen(szCfgFile, "a");
	//write GIS Raster:
	for (int j = yMin; j <= yMax; j++) {
		for (int i = xMin; i <= xMax; i++) {
			double tgtPt = rasterArray[i + j*maxW];
			if (tgtPt != world->header->NODATA) {
				ptr += sprintf(ptr, outFmtSpecifier, tgtPt);
			} else {
				ptr += sprintf(ptr, "%d ", world->header->NODATA);
			}
		}
		sprintf(ptr, "\n");
		fprintf(pRASFILE, "%s", buff);
		ptr = buff;
	}

	fclose(pRASFILE);

	delete[] buff;

	return 1;
}

struct FmtSpecHolder {
	char fmt[64];
};

template<typename T>
inline int readTable(const char * szFileName, int length, int nSkip, T *col1) {
	return readTable(szFileName, length, nSkip, col1, (int *)NULL, (int *)NULL);
}

template<typename T, typename U>
inline int readTable(const char * szFileName, int length, int nSkip, T *col1, U* col2) {
	return readTable(szFileName, length, nSkip, col1, col2, (int *)NULL);
}

template<typename T, typename U, typename V>
inline int readTable(const char * szFileName, int length, int nSkip, T *col1, U* col2, V* col3) {

	FILE 	*fIn;
	char	*szBuff, *pPtr;
	int		bRet, x, y, i;

	int		nCols = 1;
	if (col2 != nullptr) {
		nCols++;
	}
	if (col3 != nullptr) {
		nCols++;
	}

	bRet = 0;
	fIn = fopen(szFileName, "rb");
	if (fIn) {
		szBuff = (char*)malloc(_N_MAX_DYNAMIC_BUFF_LEN * sizeof(char));
		if (szBuff) {
			i = 0;
			x = 0;
			y = 0;
			bRet = 1;
			//strip away header
			for (int iHeaderLine = 0; iHeaderLine<nSkip; iHeaderLine++) {
				if (!fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
					fprintf(stderr, "ERROR: Unable to read header lines\n");
				}
			}
			/* will fail on long lines */
			while (bRet && fgets(szBuff, _N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
				if (y >= length) {
					if (bRet) {
						fprintf(stderr, "array reading: too long\n");
						printf("x=%d, y=%d\n", x, y);
						bRet = 0;
					}
				}
				pPtr = strpbrk(szBuff, "\r\n");
				if (pPtr) {
					pPtr[0] = '\0';
				} else {
					fprintf(stderr, "point data reading: line too long: should be 3 elements per line\n");
					printf("x=%d, y=%d\n", x, y);
					bRet = 0;				/* no CR or NL means line not read in fully */
				}
				pPtr = strtok(szBuff, " \t");
				while (bRet && pPtr) {
					if (x < nCols) {
						//Check if value is possible, and should be set
						std::stringstream ss;
						ss << std::string(pPtr);
						switch (x) {
							case 0:
								if (ss >> col1[y]) {
									;
								} else {
									bRet = 0;
									reportReadError("ERROR: Unable to parse value in file %s at x=%d, y=%d\n", szFileName, x, y);
								}
								break;
							case 1:
								if (ss >> col2[y]) {
									;
								} else {
									bRet = 0;
									reportReadError("ERROR: Unable to parse value in file %s at x=%d, y=%d\n", szFileName, x, y);
								}
								break;
							case 2:
								if (ss >> col3[y]) {
									;
								} else {
									bRet = 0;
									reportReadError("ERROR: Unable to parse value in file %s at x=%d, y=%d\n", szFileName, x, y);
								}
								break;
						}
					} else {
						bRet = 0;
						fprintf(stderr, "raster reading: x too large: should be 3 elements per line\n"); /* too large in x direction */
						printf("x=%d, y=%d\n", x, y);
					}
					i++;
					x++;
					pPtr = strtok(NULL, " \t");
				}
				if (x == nCols) {
					x = 0;
					y++;
				}
			}
			if (y != length) {
				if (bRet) {
					fprintf(stderr, "raster reading: y too small\n");
					printf("x=%d, y=%d\n", x, y);
					bRet = 0;
				}
			} else {
				if (bRet && i == length*nCols) {
					;
					/* all worked */
				} else {
					if (bRet) {
						fprintf(stderr, "raster reading: not read in enough points\n");
						printf("x=%d, y=%d\n", x, y);
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

template<typename T>
inline int writeTable(const char * szFileName, int length, int append, T * col1) {
	return writeTable(szFileName, length, append, col1, (int *) NULL, (int *) NULL);
}

template<typename T, typename U>
inline int writeTable(const char * szFileName, int length, int append, T * col1, U * col2) {
	return writeTable(szFileName, length, append, col1, col2, (int *) NULL);
}

template<typename T, typename U, typename V>
inline int writeTable(const char * szFileName, int length, int append, T *col1, U* col2, V* col3) {

	FILE *pFILE;

	if (append) {
		pFILE = fopen(szFileName, "a");
	} else {
		pFILE = fopen(szFileName, "w");
	}

	int nCols = 1;

	std::vector<FmtSpecHolder> colFmtSpecifiers(nCols);

	sprintf(colFmtSpecifiers[0].fmt, "%s ", OutFmt<T>::fmt());
	
	if (col2 != nullptr) {
		nCols = 2;
		colFmtSpecifiers.resize(nCols);
		sprintf(colFmtSpecifiers[nCols - 1].fmt, "%s ", OutFmt<U>::fmt());
	}

	if (col3 != nullptr) {
		nCols = 3;
		colFmtSpecifiers.resize(nCols);
		sprintf(colFmtSpecifiers[nCols - 1].fmt, "%s ", OutFmt<V>::fmt());
	}

	if (pFILE) {
		//write array:
		for (int i = 0; i < length; i++) {
			fprintf(pFILE, colFmtSpecifiers[0].fmt, col1[i]);
			if (col2 != nullptr) {
				fprintf(pFILE, colFmtSpecifiers[1].fmt, col2[i]);
			}
			if (col3 != nullptr) {
				fprintf(pFILE, colFmtSpecifiers[2].fmt, col3[i]);
			}
			fprintf(pFILE, "\n");
		}
		fclose(pFILE);
	} else {
		printf("ERROR: Unable to open %s for writing\n", szFileName);
	}
	
	return 1;
}



#endif /* _CFGUTILS_H_ */

