// ListNode.h

#pragma once

template <class T>
struct ListNode
{
	
	T* data;
	ListNode<T> *pNext;
	ListNode<T> *pPrev;

};
