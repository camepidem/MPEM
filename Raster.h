//Raster.h

//Template for raster class
#ifndef RASTER_H
#define RASTER_H 1

#include "stdafx.h"
#include "RasterHeader.h"

#pragma warning(disable : 4996)		/* stop Visual C++ 2010 from warning about C++ and thread safety when asked to compile idiomatic ANSI */

class RasterCoordinate {

public:

	RasterCoordinate(int x_ = 0, int y_ = 0) {x=x_; y=y_;};

	int x, y;

	      int& operator[](int iCoord)       { if (iCoord == 0) { return x; }; if (iCoord == 1) { return y; }; return x; };//TODO: Put back in coordinate checking/exception handling (but not necessary given this facility is never used anyway)
	const int& operator[](int iCoord) const { if (iCoord == 0) { return x; }; if (iCoord == 1) { return y; }; return x; };

};

class WorldCoordinate {

public:

	WorldCoordinate(double x_ = 0.0, double y_ = 0.0) {x=x_; y=y_;};

	double x, y;

	      double& operator[](int iCoord)       { if (iCoord == 0) { return x; }; if (iCoord == 1) { return y; }; return x; };
	const double& operator[](int iCoord) const { if (iCoord == 0) { return x; }; if (iCoord == 1) { return y; }; return x; };

};

template <class T>
class Raster {

public:

	Raster();
	template <class U>
	Raster(const RasterHeader<U> &fromHeader, bool init_ToNoData = false);
	Raster(int n_Rows, int n_Cols, double xll_Corner = 0.0, double yll_Corner = 0.0, double cell_size = 1.0, bool init_ToNoData = false);
	Raster(std::string fromFileName);
	Raster(const Raster<T> &fromRaster);
	virtual ~Raster();

	//Creation
	template <class U>
	int init(const RasterHeader<U> &fromHeader, bool init_ToNoData = false);
	int init(int n_Rows, int n_Cols, double xll_Corner = 0.0, double yll_Corner = 0.0, double cell_size = 1.0, bool init_ToNoData = false, T noData_value = RasterHeader<T>::NODATA_default);
	int init(std::string fromFileName);
	int init(const Raster<T> &fromRaster);

	//Header:
	RasterHeader<T> header;

	//Interface:
	T getValue(int x, int y) const;
	T getValueSafe(int x, int y) const;

	inline T  operator()(const int x, const int y) const {return data[x + y*header.nCols];}
	inline T& operator()(const int x, const int y)       {return data[x + y*header.nCols];}

	void setValue(int x, int y, T val);
	void setValueSafe(int x, int y, T val);

	void setAllRasterValues(T val);

	//Coordinate transformation:
	RasterCoordinate realToCell(const WorldCoordinate &worldPos) const;
	WorldCoordinate cellToReal(const RasterCoordinate &rasterdPos) const;

	vector<int> realToCell(double pX, double pY) const;
	vector<double> cellToReal(int x, int y) const;

	vector<int> realToCell(const vector<double> &realCoords) const;
	vector<double> cellToReal(const vector<int> &cellCoords) const;

protected:

	std::vector<T> data;

	int readRasterToVector(std::string fromFileName, std::vector<T> &Array, int nRows, int nCols);

};

template <class T>
Raster<T>::Raster() {
	RasterHeader<T> defaultHeader;
	init(defaultHeader, true);
}

template <class T>
template <class U>
Raster<T>::Raster(const RasterHeader<U> &fromHeader, bool init_ToNoData) {
	init(fromHeader, init_ToNoData);
}

template <class T>
Raster<T>::Raster(int n_Rows, int n_Cols, double xll_Corner, double yll_Corner, double cell_size, bool init_ToNoData) {
	init(n_Rows, n_Cols, xll_Corner, yll_Corner, cell_size, init_ToNoData);
}

template <class T>
Raster<T>::Raster(std::string fromFileName) {
	init(fromFileName);
}

template <class T>
Raster<T>::Raster(const Raster<T> &fromRaster) {
	init(fromRaster);
}

template <class T>
Raster<T>::~Raster() {
	//
}

template <class T>
int Raster<T>::init(int n_Rows, int n_Cols, double xll_Corner, double yll_Corner, double cell_size, bool init_ToNoData, T NoData_value) {

	header.init(n_Rows, n_Cols, xll_Corner, yll_Corner, cell_size);

	int nElements = header.nRows*header.nCols;

	if(nElements > 0) {

		data.resize(nElements);

		if(init_ToNoData) {
			for(int iPoint = 0; iPoint < nElements; ++iPoint) {
				//While we could do something nicer with a generic type header, we will just rely on not doing this wrong
				data[iPoint] = static_cast<T>(NoData_value);
			}
		}

		return 1;
	} else {
		return 0;
	}

}

template <class T>
template <class U>
int Raster<T>::init(const RasterHeader<U> &fromHeader, bool init_ToNoData) {

	header.init(fromHeader);

	int nElements = header.nRows*header.nCols;

	if(nElements > 0) {

		data.resize(nElements);

		if(init_ToNoData) {
			for(int iPoint = 0; iPoint < nElements; ++iPoint) {
				data[iPoint] = RasterHeader<T>::NODATA_default;
			}
		}

		return 1;

	} else {
		return 0;
	}

}

template <class T>
int Raster<T>::init(std::string fromFileName) {

	int bSuccess = header.init(fromFileName.c_str());

    int nElements = header.nRows*header.nCols;

    if(bSuccess && nElements > 0) {

        data.resize(nElements);

        bSuccess *= readRasterToVector(fromFileName, data, header.nRows, header.nCols);

    } else {
        bSuccess = 0;
    }

	return bSuccess;
}

template <class T>
int Raster<T>::init(const Raster<T> &fromRaster) {

	header.init(fromRaster.header);

	int nElements = header.nRows*header.nCols;

	if(nElements > 0) {

		data.resize(nElements);

		for(int iX = 0; iX < header.nCols; ++iX) {
			for(int iY = 0; iY < header.nRows; ++iY) {
				setValue(iX, iY, fromRaster.getValue(iX, iY));
			}
		}

		return 1;
	} else {
		return 0;
	}

}


template <class T>
T Raster<T>::getValue(int x, int y) const {
	return data[x + y*header.nCols];
}

template <class T>
T Raster<T>::getValueSafe(int x, int y) const {

	if(x >= 0 && x < header.nCols && y >= 0 && y < header.nRows) {
		return data[x + y*header.nCols];
	}

	return header.NODATA;

}

template <class T>
void Raster<T>::setValue(int x, int y, T val) {

	data[x + y*header.nCols] = val;

}

template <class T>
void Raster<T>::setValueSafe(int x, int y, T val) {

	if(x >= 0 && x < header.nCols && y >= 0 && y < header.nRows) {
		data[x + y*header.nCols] = val;
	}

}

template <class T>
void Raster<T>::setAllRasterValues(T val) {

	int nPoints = header.nRows*header.nCols;
	for(int iPoint = 0; iPoint < nPoints; ++iPoint) {
		data[iPoint] = val;
	}

}

//Coordinate transformation:
template <class T>
vector<int> Raster<T>::realToCell(double pX, double pY) const {
	//return LLC of containing cell
	vector<int> vCellCoords;
	vCellCoords.push_back(int((pX - header.xllCorner)/header.cellsize));
	vCellCoords.push_back(header.nRows - 1 - int((pY - header.yllCorner)/header.cellsize));
	return vCellCoords;
}
template <class T>
vector<double> Raster<T>::cellToReal(int x, int y) const {
	vector<double> vRealCoords;
	vRealCoords.push_back(header.xllCorner + x*header.cellsize);
	vRealCoords.push_back(header.yllCorner + ((header.nRows - 1) - y)*header.cellsize);
	return vRealCoords;
}

template <class T>
vector<int> Raster<T>::realToCell(const vector<double> &realCoords) const {
	return realToCell(realCoords[0], realCoords[1]);
}
template <class T>
vector<double> Raster<T>::cellToReal(const vector<int> &cellCoords) const {
	return cellToReal(cellCoords[0], cellCoords[1]);
}

template <class T>
RasterCoordinate Raster<T>::realToCell(const WorldCoordinate &worldPos) const {
	return realToCell(worldPos.x, worldPos.y);
}
template <class T>
WorldCoordinate Raster<T>::cellToReal(const RasterCoordinate &rasterPos) const {
	return cellToReal(rasterPos.x, rasterPos.y);
}

template <class T>
int Raster<T>::readRasterToVector(std::string fromFileName, std::vector<T> &Array, int nRows, int nCols) {
	FILE 	*fIn;
	char	*szBuff, *pPtr;
	int		bRet, x, y, i;
	T		TVal;

	const size_t N_MAX_DYNAMIC_BUFF_LEN = 131072;

	bRet = 0;
	fIn = fopen(fromFileName.c_str(), "rb");
	if (fIn) {
		szBuff = (char*)malloc(N_MAX_DYNAMIC_BUFF_LEN*sizeof(char));
		if (szBuff) {
			i = 0;
			x = 0;
			y = 0;
			bRet = 1;
			//strip away header
			for (int iHeaderLine = 0; iHeaderLine < 6; ++iHeaderLine) {
				if (!fgets(szBuff, N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
					fprintf(stderr, "ERROR: raster reading: unable to read header lines\n");
				}
			}
			/* will fail on long lines */
			while (bRet && fgets(szBuff, N_MAX_DYNAMIC_BUFF_LEN, fIn)) {
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

#endif