// DispersalKernelFunctionalUSSOD.h: interface for DispersalKernelFunctionalUSSOD class.

#ifndef DISPERSALKERNELFUNCTIONALUSSOD_H
#define DISPERSALKERNELFUNCTIONALUSSOD_H 1

#include "DispersalKernel.h"

class DispersalKernelFunctionalUSSOD : public DispersalKernel
{
public:

	DispersalKernelFunctionalUSSOD();
	virtual ~DispersalKernelFunctionalUSSOD();

	virtual int		CALL init(int iKernel);

	virtual void	CALL reset();

	//virtual int		CALL changeParameter(double dNewParameter);
	virtual int		CALL changeParameter(char *szKeyName, double dNewParameter, int bTest=0);

	virtual double	CALL KernelShort(int nDx, int nDy);

	virtual int		CALL getShortRange();
	
	virtual void	CALL KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	virtual double	CALL getVirtualSporulationProportion();

	//Used for profiling: Returns vector where element i of vector is cumulative sum of kernel from central cell to (square) range i
	virtual std::vector<double> CALL getCumulativeSumByRange();

	virtual void	CALL writeSummaryData(char *szSummaryFile);

protected:

};


#endif
