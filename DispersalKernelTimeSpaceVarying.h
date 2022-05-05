#pragma once

#include "KernelRasterPoints.h"
#include "Raster.h"
#include <unordered_map>
#include "DispersalKernel.h"

class DispersalKernelTimeSpaceVarying :	public DispersalKernel {
public:
	DispersalKernelTimeSpaceVarying();
	virtual ~DispersalKernelTimeSpaceVarying();

	virtual int		CALL init(int iKernel);

	virtual void	CALL reset();

	virtual int		CALL changeParameter(char *szKeyName, double dNewParameter, int bTest = 0);

	virtual double	CALL KernelShort(int nDx, int nDy);

	virtual int		CALL getShortRange();

	virtual void	CALL KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	virtual double	CALL getVirtualSporulationProportion();

	//Used for profiling: Returns vector where element i of vector is cumulative sum of kernel from central cell to (square) range i
	virtual std::vector<double> CALL getCumulativeSumByRange();

	virtual void	CALL writeSummaryData(char *szSummaryFile);

protected:

	std::unordered_map<int, KernelRasterPoints> kernels;
	std::vector<double> changeTimes;
	std::vector<Raster<int>> timeZones;

	std::unordered_map<int, std::string> kernelIDtoFileName;

	int nMaxLoadedKernels;

	enum class DataLoadingMode {MONOLITHIC, GRADUAL, ROLLING, END};//TODO: Clever caching-most-used few, dynamically-load-others one

	DataLoadingMode loadMode;

	size_t LoadModeRollingCurrentTimeIndex;

};

