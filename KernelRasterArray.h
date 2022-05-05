//Implementation of a dispersal kernel raster as a square array

#ifndef KERNELRASTERARRAY_H
#define KERNELRASTERARRAY_H 1

#include <functional>
#include "KernelRaster.h"

class KernelRasterArray : public KernelRaster
{
public:

	KernelRasterArray();
	KernelRasterArray(char *fromFile, int iKernel);
	KernelRasterArray(char *fromFile, int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation = false, double dWithinCellProportion = dWithinCellUnspecified);
	KernelRasterArray(int nKernelRange, std::function<double(int, int)> kernelFunction, int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation = false, double dWithinCellProportion = dWithinCellUnspecified);
	~KernelRasterArray();

	double	kernelShort(int nDx, int nDy);
	int		getShortRange();
	
	void	kernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	double	getVirtualSporulationProportion();

	static const double dWithinCellUnspecified;

	int		init(char *fromFile, int iKernel);
	int		init(char *fromFile, int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation = false , double dWithinCellProportion = dWithinCellUnspecified);
	int		init(int nKernelRange, 
				 std::function<double(int, int)> kernelFunction, 
				 int nShortRange, 
				 double dVSTransitionThreshold, 
				 bool bForceAllVirtualSporulation = false, 
				 double dWithinCellProportion = dWithinCellUnspecified);
	
	int		changeComposition(int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation = false);

	std::vector<double> getCumulativeSumByRange();

	int		writeRaster(const char *fileName);

	void	writeSummaryData(char *szSummaryFile);

protected:

	int		compose(int nShortRange, double dVSTransitionThreshold, bool bForceAllVirtualSporulation = false, double dWithinCellProportion = dWithinCellUnspecified);

	int		nKernelShortRange;
	int		nKernelLongRange;
	double	dKernelWithinCellProportion;
	double	dKernelVirtualSporulationProportion;
	vector<double> pdKernelShortArray;
	vector<double> pdKernelVirtualSporulationArray;

};


#endif
