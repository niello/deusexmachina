#pragma once
#ifndef __DEM_L1_PTR_H__
#define __DEM_L1_PTR_H__

#include <kernel/ntypes.h>

// Implements a smart pointer for CRefCounted objects. Can be used like
// a normal C++ pointer in most cases.
// Based on mangalore Ptr (C) 2003 RadonLabs GmbH

template<class T>
class Ptr
{
private:

	T* ptr;

public:

	Ptr(): ptr(NULL) {}
	Ptr(T* p): ptr(p) { if (ptr) ptr->AddRef(); }
	Ptr(const Ptr<T>& p): ptr(p.ptr) { if (ptr) ptr->AddRef(); }
	~Ptr();

	void	Create();
	bool	isvalid() const { return ptr != NULL; }
	T*		get() const;
	T*		get_unsafe() const { return ptr; }

	void	operator =(const Ptr<T>& rhs);
	void	operator =(T* rhs);
	bool	operator ==(const Ptr<T>& rhs) const { return ptr == rhs.ptr; }
	bool	operator !=(const Ptr<T>& rhs) const { return ptr != rhs.ptr; }
	bool	operator ==(const T* rhs) const { return ptr == rhs; }
	bool	operator !=(const T* rhs) const { return ptr != rhs; }
	T*		operator ->() const;
	T&		operator *() const;
	operator T*() const;
};
//---------------------------------------------------------------------

template<class T> Ptr<T>::~Ptr()
{
	if (ptr)
	{
		ptr->Release();
		ptr = NULL;
	}
}
//---------------------------------------------------------------------

template<class T>
//Ptr<T>&
void Ptr<T>::operator =(const Ptr<T>& Other)
{
	if (ptr != Other.ptr)
	{
		T* NewPtr = Other.ptr;
		if (NewPtr) NewPtr->AddRef();
		if (ptr) ptr->Release();		// Here Other can be destructed so we remember it's Ptr before
		ptr = NewPtr;
	}
	//return *this;
}
//---------------------------------------------------------------------

template<class T>
//Ptr<T>&
void Ptr<T>::operator =(T* Ptr)
{
	if (ptr != Ptr)
	{
		if (Ptr) Ptr->AddRef();
		if (ptr) ptr->Release();
		ptr = Ptr;
	}
	//return *this;
}
//---------------------------------------------------------------------

template<class T> inline T* Ptr<T>::operator->() const
{
	n_assert2(ptr, "NULL pointer access in Ptr::operator->()!");
	return ptr;
}
//---------------------------------------------------------------------

template<class T> inline T& Ptr<T>::operator*() const
{
	n_assert2(ptr, "NULL pointer access in Ptr::operator*()!");
	return *ptr;
}
//---------------------------------------------------------------------

template<class T> inline Ptr<T>::operator T*() const
{
	n_assert2(ptr, "NULL pointer access in Ptr::operator T*()!");
	return ptr;
}
//---------------------------------------------------------------------

template<class T> void Ptr<T>::Create()
{
	n_assert(!ptr);
	ptr = n_new(T);
	ptr->AddRef();
}
//---------------------------------------------------------------------

template<class T> inline T* Ptr<T>::get() const
{
	n_assert2(ptr, "NULL pointer access in Ptr::get()!");
	return ptr;
}
//---------------------------------------------------------------------

#endif





