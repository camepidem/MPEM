// DllWrapperFactory.h: interface for the DllWrapperFactory class.

#ifndef DLLWRAPPERFACTORY_H
#define DLLWRAPPERFACTORY_H 1

class DispersalKernel;
class DispersalKernelGeneric;

class DllWrapperFactory  
{
public:
	DllWrapperFactory();
	virtual ~DllWrapperFactory();
	virtual DispersalKernel * __stdcall	CreateProduct()=0;
protected:
	virtual DispersalKernelGeneric * __stdcall CreateDispersalKernelGeneric()=0;
};

#endif
