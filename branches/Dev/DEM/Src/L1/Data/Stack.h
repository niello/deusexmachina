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
	T			Pop();
	const T&	Top() const { return Storage.Back(); }
	bool		IsEmpty() const { return Storage.IsEmpty(); }
};

template<class T>
inline T CStack<T>::Pop()
{
	if (Storage.IsEmpty()) return T();
	T Elm = Storage.Back();
	Storage.RemoveAt(Storage.GetCount() - 1);
	return Elm;
}
//---------------------------------------------------------------------

#endif

