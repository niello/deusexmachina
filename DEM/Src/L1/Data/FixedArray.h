#pragma once
#ifndef __DEM_L1_FIXED_ARRAY_H__
#define __DEM_L1_FIXED_ARRAY_H__

#include <System/System.h>
#include <Data/ArrayUtils.h>	// Search and closest index
#include <algorithm>			// std::sort

// A fixed count, bounds-checked array.

template<class T>
class CFixedArray
{
private:

	T*		pData;
	UPTR	Count;

	void	Copy(const CFixedArray<T>& src);
	void	Delete() { SAFE_DELETE_ARRAY(pData); Count = 0; }

public:

	typedef T* CIterator;

	CFixedArray(): Count(0), pData(NULL) {}
	CFixedArray(UPTR _Size): Count(0), pData(NULL) { SetSize(_Size); }
	CFixedArray(const CFixedArray<T>& Other): Count(0), pData(NULL) { Copy(Other); }
	~CFixedArray() { if (pData) n_delete_array(pData); }

	CIterator	Begin() const { return pData; }
	CIterator	End() const { return pData + Count; }
	CIterator	IteratorAt(IPTR Idx) const { return Idx == INVALID_INDEX ? NULL : pData + Idx; }

	void		Clear(T Elm = T()) { for (UPTR i = 0; i < Count; ++i) pData[i] = Elm; }
	CIterator	Find(const T& Val) const { IPTR Idx = FindIndex(Val); return Idx == INVALID_INDEX ? End() : IteratorAt(Idx); }
	IPTR		FindIndex(const T& Elm) const;
	CIterator	FindSorted(const T& Val) const { IPTR Idx = ArrayUtils::FindIndexSorted(pData, Count, Val); return Idx == INVALID_INDEX ? End() : IteratorAt(Idx); }
	IPTR		FindIndexSorted(const T& Val) const { return ArrayUtils::FindIndexSorted(pData, Count, Val); }
	bool		Contains(const T& Elm) const { return FindIndex(Elm) != -1; }
	void		Sort() { std::sort(pData, pData + Count); }
	template<class TCmp>
	void		Sort() { std::sort(pData, pData + Count, TCmp()); }

	void		SetSize(UPTR NewSize, bool KeepValues = false);
	UPTR		GetCount() const { return Count; }
	T*			GetPtr() { return pData; }

	void		RawCopyFrom(const T* pSrc, UPTR SrcCount);

	void		operator =(const CFixedArray<T>& Other) { Copy(Other); }
	T&			operator [](UPTR Index) const;
};

template<class T> void CFixedArray<T>::SetSize(UPTR NewSize, bool KeepValues)
{
	if (NewSize == Count) return;

	if (KeepValues)
	{
		T* pOldData = pData;
		pData = (NewSize > 0) ? n_new_array(T, NewSize) : NULL;
		UPTR MinSize = n_min(NewSize, Count);
		for (UPTR i = 0; i < MinSize; ++i) pData[i] = pOldData[i];
		if (pOldData) n_delete_array(pOldData);
	}
	else
	{
		if (pData) n_delete_array(pData);
		pData = (NewSize > 0) ? n_new_array(T, NewSize) : NULL;
	}

	Count = NewSize;
}
//---------------------------------------------------------------------

template<class T> void CFixedArray<T>::Copy(const CFixedArray<T>& Other)
{
	if (this == &Other) return;
	SetSize(Other.Count);
	for (UPTR i = 0; i < Count; ++i) pData[i] = Other.pData[i];
}
//---------------------------------------------------------------------

// NB: Doesn't call element constructors
template<class T> void CFixedArray<T>::RawCopyFrom(const T* pSrc, UPTR SrcCount)
{
	if (!pSrc) return;
	SetSize(SrcCount);
	if (SrcCount) memcpy(pData, pSrc, SrcCount * sizeof(T));
}
//---------------------------------------------------------------------

template<class T> IPTR CFixedArray<T>::FindIndex(const T& Elm) const
{
	for (UPTR i = 0; i < Count; ++i)
		if (Elm == pData[i]) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

template<class T> inline T& CFixedArray<T>::operator [](UPTR Index) const
{
	n_assert(pData && Index < Count);
	return pData[Index];
}
//---------------------------------------------------------------------

#endif
