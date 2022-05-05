// LinkedList.h

#pragma once

#include "ListNode.h"

template <class T>
class LinkedList
{


public:

	LinkedList();
	~LinkedList();
	ListNode<T>* addAsFirst(T *data);
	ListNode<T>* excise(ListNode<T> *node);

	int nLength;

	ListNode<T> *pFirst;

};

template <class T>
LinkedList<T>::LinkedList()
{
	pFirst = new ListNode<T>;
	pFirst->pNext = NULL;
	pFirst->pPrev = NULL;
	pFirst->data = NULL;

	nLength = 0;
}

template <class T>
LinkedList<T>::~LinkedList()
{

	//Deconstruct the linked list:
	//Remove all nodes:
	while(pFirst->pNext != NULL) {
		excise(pFirst->pNext);
	}

	//Delete anchor node:
	delete pFirst;
	pFirst = NULL;

}

template <class T>
ListNode<T>* LinkedList<T>::addAsFirst(T* data)
{
	ListNode<T> *q;

	q = new ListNode<T>;
	
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

template <class T>
ListNode<T>* LinkedList<T>::excise(ListNode<T> *node) {

	nLength--;

	node->pPrev->pNext = node->pNext;
	if(node->pNext != NULL) {
		node->pNext->pPrev = node->pPrev;
	}

	delete node;

	return NULL;
}


