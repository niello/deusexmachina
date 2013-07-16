#pragma once
#ifndef __DEM_L1_PTR_H__
#define __DEM_L1_PTR_H__

#include <StdDEM.h>

// Smart pointer class which manages the life time of CRefCounted objects.
// Can be used like a normal C++ pointer in most cases.

template<class T>
class Ptr
{
private:

	T* pObj;

public:

	Ptr(): pObj(NULL) {}
	Ptr(T* pSrcObj): pObj(pSrcObj) { if (pObj) pObj->AddRef(); }
	Ptr(const Ptr<T>& pSrcPtr): pObj(pSrcPtr.pObj) { if (pObj) pObj->AddRef(); }
	~Ptr() { SAFE_RELEASE(pObj); }

	bool	IsValid() const { return !!pObj; }
	T*		Get() const;
	T*		GetUnsafe() const { return pObj; }

	void	operator =(const Ptr<T>& Other);
	void	operator =(T* Other);
	bool	operator ==(const Ptr<T>& Other) const { return pObj == Other.pObj; }
	bool	operator !=(const Ptr<T>& Other) const { return pObj != Other.pObj; }
	bool	operator ==(const T* pOtherObj) const { return pObj == pOtherObj; }
	bool	operator !=(const T* pOtherObj) const { return pObj != pOtherObj; }
	T*		operator ->() const;
	T&		operator *() const;
			operator T*() const;
};
//---------------------------------------------------------------------

template<class T>
void Ptr<T>::operator =(const Ptr<T>& Other)
{
	if (pObj != Other.pObj)
	{
		T* NewPtr = Other.pObj;
		if (NewPtr) NewPtr->AddRef();
		if (pObj) pObj->Release();		// Here Other can be destructed so we remember it's Ptr before
		pObj = NewPtr;
	}
}
//---------------------------------------------------------------------

template<class T>
void Ptr<T>::operator =(T* Ptr)
{
	if (pObj != Ptr)
	{
		if (Ptr) Ptr->AddRef();
		if (pObj) pObj->Release();
		pObj = Ptr;
	}
}
//---------------------------------------------------------------------

template<class T> inline T* Ptr<T>::operator->() const
{
	n_assert2(pObj, "NULL pointer access in Ptr::operator->()!");
	return pObj;
}
//---------------------------------------------------------------------

template<class T> inline T& Ptr<T>::operator*() const
{
	n_assert2(pObj, "NULL pointer access in Ptr::operator*()!");
	return *pObj;
}
//---------------------------------------------------------------------

template<class T> inline Ptr<T>::operator T*() const
{
	n_assert2(pObj, "NULL pointer access in Ptr::operator T*()!");
	return pObj;
}
//---------------------------------------------------------------------

template<class T> inline T* Ptr<T>::Get() const
{
	n_assert2(pObj, "NULL pointer access in Ptr::Get()!");
	return pObj;
}
//---------------------------------------------------------------------

#endif





