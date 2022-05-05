// RasterHeader.h

#pragma once

template <class T>
class RasterHeader {

public:

	RasterHeader();
	~RasterHeader();

	int init(const char *fromFileName);
	template <class U>
	int init(const RasterHeader<U> &fromHeader);
	int init(int n_Rows, int n_Cols, double xll_Corner = 0.0, double yll_Corner = 0.0, double cell_size = 1.0, T no_data_value = NODATA_default);

	template<class U>
	int sameAs(const RasterHeader<U> &targetHeader) const;

	int nRows, nCols;
	double xllCorner, yllCorner, cellsize;
	T NODATA;

	static const T NODATA_default;

protected:



};

//Have to include after declaration of RasterHeader<T>, as otherwise a horrible include loop would make Landscape use RasterHeader<int> before it was declared...
#include "cfgutils.h"

template <class T>
RasterHeader<T>::RasterHeader() {
	init(1, 1);
}

template <class T>
RasterHeader<T>::~RasterHeader() {
	//
}

template <class T>
int RasterHeader<T>::init(const char *fromFileName) {

	int bSuccess = 1;

	bSuccess *= readIntFromCfg(fromFileName, "ncols", &nCols);
	bSuccess *= readIntFromCfg(fromFileName, "nrows", &nRows);
	bSuccess *= readDoubleFromCfg(fromFileName, "xllcorner", &xllCorner);
	bSuccess *= readDoubleFromCfg(fromFileName, "yllcorner", &yllCorner);
	bSuccess *= readDoubleFromCfg(fromFileName, "cellsize", &cellsize);

	//Backwards compatibility with rasters that have a technically incorrect header:
	char tmpNODATA[256];
	int bFoundANODATA = 0;
	if (readStringFromCfg(fromFileName, "NODATA_value", tmpNODATA)) {
		bFoundANODATA = 1;
	}
	if (readStringFromCfg(fromFileName, "NODATA", tmpNODATA)) {
		bFoundANODATA = 1;
	}

	std::string sNODATA = std::string(tmpNODATA);
	std::stringstream ssND;
	ssND << sNODATA;
	ssND >> NODATA;

	bSuccess *= bFoundANODATA;

	return bSuccess;
}

//TODO: should be ok to just use the default copy constructor in general
template <class T>
template <class U>
int RasterHeader<T>::init(const RasterHeader<U> &fromHeader) {

	nRows = fromHeader.nRows;
	nCols = fromHeader.nCols;
	xllCorner = fromHeader.xllCorner;
	yllCorner = fromHeader.yllCorner;
	cellsize = fromHeader.cellsize;
	NODATA = RasterHeader<T>::NODATA_default;

	return 1;
}

template <class T>
int RasterHeader<T>::init(int n_Rows, int n_Cols, double xll_Corner, double yll_Corner, double cell_size, T no_data_value) {

	nRows = n_Rows;
	nCols = n_Cols;
	xllCorner = xll_Corner;
	yllCorner = yll_Corner;
	cellsize = cell_size;
	NODATA = no_data_value;

	return 1;
}

template <class T>
template <class U>
int RasterHeader<T>::sameAs(const RasterHeader<U> &targetHeader) const {

	int bMatch = 1;

	//Tolerance of ~1% of cellsize
	double dTolerance = 0.01*cellsize;
	//irrelevant which cellsize we use to establish tolerance as they must be within 1% of each other

	if (nRows != targetHeader.nRows) {
		bMatch = 0;
	}

	if (nCols != targetHeader.nCols) {
		bMatch = 0;
	}

	if (abs(cellsize - targetHeader.cellsize) > dTolerance) {
		bMatch = 0;
	}

	if (abs(xllCorner - targetHeader.xllCorner) > dTolerance) {
		bMatch = 0;
	}

	if (abs(yllCorner - targetHeader.yllCorner) > dTolerance) {
		bMatch = 0;
	}


	return bMatch;
}
