//LinkedList.h

#ifndef LINKEDLISTLOCATION_H
#define LINKEDLISTLOCATION_H 1

#include "classes.h"
#include "ListNodeLocation.h"

class LinkedListLocation
{


public:

	LinkedListLocation();
	~LinkedListLocation();

	ListNodeLocation* addAsFirst(LocationMultiHost* data);
	ListNodeLocation* excise(ListNodeLocation *node);

	int nLength;

	ListNodeLocation *pFirst;

};

#endif
