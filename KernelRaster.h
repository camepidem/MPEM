//Abstract implementation of a dispersal kernel raster

#ifndef KERNELRASTER_H
#define KERNELRASTER_H 1

class KernelRaster
{
public:

	KernelRaster();
	virtual ~KernelRaster();

	static KernelRaster *createKernelRasterFromFile(char *fromFile, int iKernel);

	virtual double	kernelShort(int nDx, int nDy);
	virtual int		getShortRange();
	
	virtual void	kernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	virtual double	getVirtualSporulationProportion();

};


#endif
