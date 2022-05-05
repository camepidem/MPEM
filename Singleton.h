// Singleton.h: interface for the CSingleton class.

#ifndef SINGLETON_H
#define SINGLETON_H 1

/* good old meyers' singleton */

template<class T>
class CSingleton 
{
public:
	static T& Instance() 
	{
		static T theSingleton;
		return theSingleton;
	}
/* more (non-static) functions here */
private:
	CSingleton();								// ctor hidden
	CSingleton(CSingleton const&);				// copy ctor hidden
	CSingleton& operator=(CSingleton const&);	// assign op. hidden
	~CSingleton();								// dtor hidden
};

#endif
