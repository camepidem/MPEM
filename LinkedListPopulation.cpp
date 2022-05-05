//LinkedListPopulation.cpp

#include "stdafx.h"
#include "LinkedListPopulation.h"

LinkedListPopulation::LinkedListPopulation()
{
	pFirst = new ListNodePopulation;
	pFirst->pNext = NULL;
	pFirst->pPrev = NULL;
	pFirst->data = NULL;

	nLength = 0;
}

LinkedListPopulation::~LinkedListPopulation()
{

	//Deconstruct the linked list:
	//Remove all nodes:
	while(pFirst->pNext != NULL) {
		excise(pFirst->pNext);
	}

	//Delete anchor node:
	delete pFirst;

}

ListNodePopulation* LinkedListPopulation::addAsFirst(Population* data)
{
	ListNodePopulation *q;

	q = new ListNodePopulation;
	
	q->data = data;

	q->pPrev = pFirst;
	q->pNext = pFirst->pNext;
	if(q->pNext != NULL) {
		q->pNext->pPrev = q;
	}
	pFirst->pNext = q;

	nLength++;

	return q;

}

ListNodePopulation* LinkedListPopulation::excise(ListNodePopulation *node) {

	nLength--;

	node->pPrev->pNext = node->pNext;
	if(node->pNext != NULL) {
		node->pNext->pPrev = node->pPrev;
	}

	delete node;

	return NULL;
}
