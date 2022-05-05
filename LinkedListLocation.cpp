//LinkedListLocation.cpp

#include "stdafx.h"
#include "LinkedListLocation.h"

LinkedListLocation::LinkedListLocation()
{
	pFirst = new ListNodeLocation;
	pFirst->pNext = NULL;
	pFirst->pPrev = NULL;
	pFirst->data = NULL;

	nLength = 0;
}

LinkedListLocation::~LinkedListLocation()
{

	//Deconstruct the linked list:
	//Remove all nodes:
	while(pFirst->pNext != NULL) {
		excise(pFirst->pNext);
	}

	//Delete anchor node:
	delete pFirst;

}

ListNodeLocation* LinkedListLocation::addAsFirst(LocationMultiHost* data)
{
	ListNodeLocation *q;

	q = new ListNodeLocation;
	
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

ListNodeLocation* LinkedListLocation::excise(ListNodeLocation *node) {

	nLength--;

	node->pPrev->pNext = node->pNext;
	if(node->pNext != NULL) {
		node->pNext->pPrev = node->pPrev;
	}

	delete node;

	return NULL;
}
