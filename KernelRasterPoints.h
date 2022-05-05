//Implementation of a dispersal kernel raster as a points list

#ifndef KERNELRASTERPOINTS_H
#define KERNELRASTERPOINTS_H 1

#include "KernelRaster.h"

class KernelRasterPoints : public KernelRaster
{
public:

	KernelRasterPoints();
	KernelRasterPoints(const char *fromFile);
	~KernelRasterPoints();

	int init(const char *fromFile);

	double	kernelShort(int nDx, int nDy);
	int		getShortRange();
	
	void	kernelVirtualSporulation(double tNow, double sourceX, double sourceY, double &dDx, double &dDy);
	double	getVirtualSporulationProportion();

protected:

	int		nKernelPoints;
	double  dCellsizeRatio; //Points files can be of different resolution, using smaller or larger cells than the landscape grid
	double  dTotalRelease; //deposition specified in points file may only be a fraction of what was actually released, so may need to know what fraction of sporulation never lands
	vector<double> pdKernelVirtualSporulationArray;
	vector<double> pdKernelVirtualSporulationArrayCoordsX;
	vector<double> pdKernelVirtualSporulationArrayCoordsY;

	//TODO: Maybe allow explicit coupling

	int		readKernelRasterPointsfromfile(const char *fromFile);

};


#endif
