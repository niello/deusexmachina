#pragma once
#ifndef __DEM_L1_STACK_H__
#define __DEM_L1_STACK_H__

#include <util/narray.h>

// Array-based stack class

template<class T>
class CStack
{
private:

	nArray<T> Storage;

public:

	void		Push(const T& Elm) { Storage.Append(Elm); }
	T			Pop();
	const T&	Top() const { return Storage.Back(); }
	bool		IsEmpty() const { return Storage.IsEmpty(); }
};

template<class T>
inline T CStack<T>::Pop()
{
	if (Storage.IsEmpty()) return T();
	T Elm = Storage.Back();
	Storage.EraseAt(Storage.GetCount() - 1);
	return Elm;
}
//---------------------------------------------------------------------

#endif

