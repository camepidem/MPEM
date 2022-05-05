// DispersalKernel.h: abstract interface for Dispersal Kernel Classes

#include <vector>

//Apparently GCC does not like stdcall...
#ifdef _WIN32
#define CALL __stdcall
#else
#define CALL 
#endif

#ifndef DISPERSALKERNEL_H
#define DISPERSALKERNEL_H 1

class DispersalKernel
{
public:

	DispersalKernel();
	virtual ~DispersalKernel();

	virtual int		CALL init(int iKernel)=0;

	//virtual int		CALL changeParameter(double dNewParameter)=0;
	virtual int		CALL changeParameter(char *szKeyName, double dNewParameter, int bTest=0)=0;

	virtual void	CALL reset()=0;

	virtual double	CALL KernelShort(int nDx, int nDy)=0;

	virtual int		CALL getShortRange()=0;
	
	virtual void	CALL KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy)=0;
	virtual double	CALL getVirtualSporulationProportion()=0;

	//Used for profiling: Returns vector where element i of vector is cumulative sum of kernel from central cell to (square) range i
	virtual std::vector<double> CALL getCumulativeSumByRange()=0;

	virtual void	CALL writeSummaryData(char *szSummaryFile)=0;

};


#endif
