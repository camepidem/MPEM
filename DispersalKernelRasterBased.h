// DispersalKernelRasterBased.h: interface for DispersalKernelRasterBased class.

#ifndef DISPERSALKERNELRASTERBASED_H
#define DISPERSALKERNELRASTERBASED_H 1

#include "KernelRasterArray.h"
#include "DispersalKernel.h"

struct SKernelRasterInitData {

	char cKernelType;

	int iKernel;

	int bSpecifiedWithinCellProportion;
	double dKernelWithinCellProportion;
	double dKernelParameter;
	double dKernelScaling;
	int nKernelLongRange;
	int nKernelShortRange;

	double dKernelLongCutoff;
	int nKernelLongCutoffRange;
	int bKernelLongCutoffByRange;
	int bForceAllVirtualSporulation;
	double dKernelVirtualSporulationProportion;

	int bCopy;

};

class DispersalKernelRasterBased : public DispersalKernel
{
public:

	DispersalKernelRasterBased();
	virtual ~DispersalKernelRasterBased();

	virtual int		CALL init(int iKernel);
	virtual int		CALL initFromDataStruct(int bMessages=0);

	//virtual int		CALL changeParameter(double dNewParameter);
	virtual int		CALL changeParameter(char *szKeyName, double dNewParameter, int bTest=0);

	virtual void	CALL reset();

	virtual double	CALL KernelShort(int nDx, int nDy);

	virtual int		CALL getShortRange();
	
	virtual void	CALL KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	virtual double	CALL getVirtualSporulationProportion();

	//Used for profiling: Returns vector where element i of vector is cumulative sum of kernel from central cell to (square) range i
	virtual std::vector<double> CALL getCumulativeSumByRange();

	virtual void	CALL writeSummaryData(char *szSummaryFile);

protected:

	SKernelRasterInitData SKernelData;

	KernelRasterArray kernelRaster;

	double expKernel(int dx, int dy);
	double powKernel(int dx, int dy);
	double adjKernel(int dx, int dy);

};


#endif
