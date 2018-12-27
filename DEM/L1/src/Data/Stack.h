#pragma once
#ifndef __DEM_L1_STACK_H__
#define __DEM_L1_STACK_H__

#include <Data/Array.h>

// Array-based stack class

template<class T>
class CStack
{
private:

	CArray<T> Storage;

public:

	void		Push(const T& Elm) { Storage.Add(Elm); }
	bool		Pop(T* pOutValue = NULL);
	const T&	Top() const { return Storage.Back(); }
	bool		IsEmpty() const { return Storage.IsEmpty(); }
};

template<class T>
inline bool CStack<T>::Pop(T* pOutValue)
{
	if (Storage.IsEmpty()) FAIL;
	Storage.Remove(Storage.GetCount() - 1, pOutValue);
	OK;
}
//---------------------------------------------------------------------

#endif

