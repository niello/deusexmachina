#pragma once
#ifndef __DEM_L1_FIXED_ARRAY_H__
#define __DEM_L1_FIXED_ARRAY_H__

#include <StdDEM.h>
#include <algorithm> // std::sort

// A fixed count, bounds checked array.

template<class T>
class CFixedArray
{
private:

	T*		pData;
	DWORD	Count;

	void	Allocate(DWORD NewSize);
	void	Copy(const CFixedArray<T>& src);
	void	Delete();

public:

	typedef T* CIterator;

	CFixedArray(): Count(0), pData(NULL) {}
	CFixedArray(DWORD _Size): Count(0), pData(NULL) { Allocate(_Size); }
	CFixedArray(const CFixedArray<T>& Other): Count(0), pData(NULL) { Copy(Other); }
	~CFixedArray() { if (pData) n_delete_array(pData); }

	CIterator	Begin() const { return pData; }
	CIterator	End() const { return pData + Count; }

	void		Clear(T Elm = T()) { for (DWORD i = 0; i < Count; ++i) pData[i] = Elm; }
	int			FindIndex(const T& Elm) const;
	bool		Contains(const T& Elm) const { return FindIndex(Elm) != -1; }
	void		Sort() { std::sort(pData, pData + Count); }
	template<class TCmp>
	void		Sort() { std::sort(pData, pData + Count, TCmp()); }

	void		SetSize(DWORD NewSize) { if (Count != NewSize) Allocate(NewSize); }
	DWORD		GetCount() const { return Count; }
	T*			GetPtr() { return pData; }

	void		RawCopyFrom(const T* pSrc, DWORD SrcCount);

	void	operator =(const CFixedArray<T>& Other) { Copy(Other); }
	T&		operator [](int Index) const;
};

template<class T> void CFixedArray<T>::Allocate(DWORD NewSize)
{
	if (NewSize == Count) return;
	if (pData) n_delete_array(pData);
	Count = NewSize;
	pData = (NewSize > 0) ? n_new_array(T, NewSize) : NULL;
}
//---------------------------------------------------------------------

template<class T> void CFixedArray<T>::Copy(const CFixedArray<T>& Other)
{
	if (this == &Other) return;
	Allocate(Other.Count);
	for (DWORD i = 0; i < Count; ++i) pData[i] = Other.pData[i];
}
//---------------------------------------------------------------------

// Doesn't call element constructors
template<class T> void CFixedArray<T>::RawCopyFrom(const T* pSrc, DWORD SrcCount)
{
	n_assert(pSrc);
	memcpy(pData, pSrc, n_min(SrcCount, Count));
}
//---------------------------------------------------------------------

template<class T> void CFixedArray<T>::Delete()
{
	if (pData)
	{
		n_delete_array(pData);
		pData = NULL;
	}
	Count = 0;
}
//---------------------------------------------------------------------

template<class T> T& CFixedArray<T>::operator [](int Index) const
{
	n_assert(pData && Index >= 0 && Index < (int)Count);
	return pData[Index];
}
//---------------------------------------------------------------------

template<class T> int CFixedArray<T>::FindIndex(const T& Elm) const
{
	for (DWORD i = 0; i < Count; ++i)
		if (Elm == pData[i]) return i;
	return -1;
}
//---------------------------------------------------------------------

#endif
