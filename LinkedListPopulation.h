//LinkedListPopulation.h

#ifndef LINKEDLIST_H
#define LINKEDLIST_H 1

#include "classes.h"
#include "ListNodePopulation.h"

class LinkedListPopulation
{


public:

	LinkedListPopulation();
	~LinkedListPopulation();
	ListNodePopulation* addAsFirst(Population* data);
	ListNodePopulation* excise(ListNodePopulation *node);

	int nLength;

	ListNodePopulation *pFirst;

};

#endif
