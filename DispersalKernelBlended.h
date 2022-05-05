#pragma once

#include "DispersalKernel.h"

class DispersalKernelBlended : public DispersalKernel {
public:
	DispersalKernelBlended();
	virtual ~DispersalKernelBlended();

	virtual int		CALL init(int iKernel);

	virtual void	CALL reset();

	virtual int		CALL changeParameter(char *szKeyName, double dNewParameter, int bTest = 0);

	virtual double	CALL KernelShort(int nDx, int nDy);

	virtual int		CALL getShortRange();

	virtual void	CALL KernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	virtual double	CALL getVirtualSporulationProportion();

	virtual void	CALL writeSummaryData(char *szSummaryFile);

	//Used for profiling: Returns vector where element i of vector is cumulative sum of kernel from central cell to (square) range i
	virtual std::vector<double> CALL getCumulativeSumByRange();

	static int		CALL getTargetKernelData(int iKernel, vector<int> &indices, vector<double> &weights);

protected:

	vector<int> indices; //Blended kernel does not own kernels, they are being referenced from DispersalManager
	vector<double> weights;

};

