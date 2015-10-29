#pragma once
#ifndef __DEM_L1_FIXED_ARRAY_H__
#define __DEM_L1_FIXED_ARRAY_H__

#include <System/System.h>
#include <algorithm> // std::sort

// A fixed count, bounds-checked array.

template<class T>
class CFixedArray
{
private:

	T*		pData;
	UPTR	Count;

	void	Allocate(UPTR NewSize);
	void	Copy(const CFixedArray<T>& src);
	void	Delete();

public:

	typedef T* CIterator;

	CFixedArray(): Count(0), pData(NULL) {}
	CFixedArray(UPTR _Size): Count(0), pData(NULL) { Allocate(_Size); }
	CFixedArray(const CFixedArray<T>& Other): Count(0), pData(NULL) { Copy(Other); }
	~CFixedArray() { if (pData) n_delete_array(pData); }

	CIterator	Begin() const { return pData; }
	CIterator	End() const { return pData + Count; }

	void		Clear(T Elm = T()) { for (UPTR i = 0; i < Count; ++i) pData[i] = Elm; }
	int			FindIndex(const T& Elm) const;
	//!!!need find sorted!
	bool		Contains(const T& Elm) const { return FindIndex(Elm) != -1; }
	void		Sort() { std::sort(pData, pData + Count); }
	template<class TCmp>
	void		Sort() { std::sort(pData, pData + Count, TCmp()); }

	void		SetSize(UPTR NewSize) { Allocate(NewSize); }
	UPTR		GetCount() const { return Count; }
	T*			GetPtr() { return pData; }

	void		RawCopyFrom(const T* pSrc, UPTR SrcCount);

	void		operator =(const CFixedArray<T>& Other) { Copy(Other); }
	T&			operator [](UPTR Index) const;
};

template<class T> void CFixedArray<T>::Allocate(UPTR NewSize)
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
	for (UPTR i = 0; i < Count; ++i) pData[i] = Other.pData[i];
}
//---------------------------------------------------------------------

// NB: Doesn't call element constructors
template<class T> void CFixedArray<T>::RawCopyFrom(const T* pSrc, UPTR SrcCount)
{
	if (!pSrc) return;
	Allocate(SrcCount);
	if (SrcCount) memcpy(pData, pSrc, SrcCount * sizeof(T));
}
//---------------------------------------------------------------------

template<class T> void CFixedArray<T>::Delete()
{
	SAFE_DELETE_ARRAY(pData);
	Count = 0;
}
//---------------------------------------------------------------------

template<class T> int CFixedArray<T>::FindIndex(const T& Elm) const
{
	for (UPTR i = 0; i < Count; ++i)
		if (Elm == pData[i]) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

template<class T> T& CFixedArray<T>::operator [](UPTR Index) const
{
	n_assert(pData && Index < Count);
	return pData[Index];
}
//---------------------------------------------------------------------

#endif
