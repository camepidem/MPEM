// Factory.h: interface for the Factory class.

#ifndef FACTORY_H
#define FACTORY_H 1

#include "DllWrapperFactory.h"
#include "Singleton.h"

class Factory : public DllWrapperFactory  
{
public:
	Factory();
	virtual ~Factory();
	DispersalKernel * __stdcall	CreateProduct();		
protected:
	DispersalKernelGeneric * __stdcall CreateDispersalKernelGeneric();
};

typedef CSingleton<Factory> FactorySingleton;

#endif
