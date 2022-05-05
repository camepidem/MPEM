// DispersalKernelGeneric.h: interface for DispersalKernelGeneric class.

#ifndef DISPERSALKERNELGENERIC_H
#define DISPERSALKERNELGENERIC_H 1

#include "DispersalKernel.h"

class DispersalKernelGeneric : public DispersalKernel
{
public:

	DispersalKernelGeneric();
	virtual ~DispersalKernelGeneric();

	virtual int		CALL init(int iKernel);

	virtual void	CALL reset();

	virtual double	CALL KernelShort(int nDx, int nDy);

	virtual int		CALL getShortRange();
	
	virtual void	CALL KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	virtual double	CALL getVirtualSporulationProportion();

	//Used for profiling: Returns vector where element i of vector is cumulative sum of kernel from central cell to (square) range i
	virtual std::vector<double> CALL getCumulativeSumByRange();

	virtual void	CALL writeSummaryData(char *szSummaryFile);

};


#endif
