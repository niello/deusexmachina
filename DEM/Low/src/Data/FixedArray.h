#pragma once
#ifndef __DEM_L1_FIXED_ARRAY_H__
#define __DEM_L1_FIXED_ARRAY_H__

#include <System/System.h>
#include <Data/ArrayUtils.h>	// Search and closest index
#include <algorithm>			// std::sort

// A fixed count, bounds-checked array.

template<typename T, typename S = UPTR>
class CFixedArray
{
private:

	T* pData = nullptr;
	S  Count = 0;

	void	Copy(const CFixedArray<T, S>& src);
	void	Delete() { SAFE_DELETE_ARRAY(pData); Count = 0; }

public:

	typedef T* CIterator;

	CFixedArray() = default;
	CFixedArray(S _Size) { SetSize(_Size); }
	CFixedArray(const CFixedArray<T, S>& Other) { Copy(Other); }
	CFixedArray(CFixedArray<T, S>&& Other) : pData(Other.pData), Count(Other.Count) { Other.pData = nullptr; Other.Count = 0; }
	~CFixedArray() { if (pData) n_delete_array(pData); }

	CIterator	IteratorAt(IPTR Idx) const { return Idx == INVALID_INDEX ? nullptr : pData + Idx; }

	void		Clear(T Elm = T()) { for (S i = 0; i < Count; ++i) pData[i] = Elm; }
	CIterator	Find(const T& Val) const { IPTR Idx = FindIndex(Val); return Idx == INVALID_INDEX ? End() : IteratorAt(Idx); }
	IPTR		FindIndex(const T& Elm) const;
	CIterator	FindSorted(const T& Val) const { IPTR Idx = ArrayUtils::FindIndexSorted(pData, Count, Val); return Idx == INVALID_INDEX ? End() : IteratorAt(Idx); }
	IPTR		FindIndexSorted(const T& Val) const { return ArrayUtils::FindIndexSorted(pData, Count, Val); }
	bool		Contains(const T& Elm) const { return FindIndex(Elm) != -1; }
	void		Sort() { std::sort(pData, pData + Count); }
	template<class TCmp>
	void		Sort() { std::sort(pData, pData + Count, TCmp()); }

	void		SetSize(S NewSize, bool KeepValues = false);

	// std C++ compatibility
	T*          begin() { return pData; }
	const T*    begin() const { return pData; }
	const T*    cbegin() const { return pData; }
	T*          end() { return pData + Count; }
	const T*    end() const { return pData + Count; }
	const T*    cend() const { return pData + Count; }
	S		    size() const { return Count; }
	T*          data() { return pData; }
	const T*    data() const { return pData; }

	void		RawCopyFrom(const T* pSrc, S SrcCount);

	void		operator =(const CFixedArray<T, S>& Other) { Copy(Other); }
	T&			operator [](S Index) const;
};

template<typename T, typename S> void CFixedArray<T, S>::SetSize(S NewSize, bool KeepValues)
{
	if (NewSize == Count) return;

	if (KeepValues)
	{
		T* pOldData = pData;
		pData = (NewSize > 0) ? n_new_array(T, NewSize) : nullptr;
		S MinSize = std::min(NewSize, Count);
		for (S i = 0; i < MinSize; ++i) pData[i] = pOldData[i];
		if (pOldData) n_delete_array(pOldData);
	}
	else
	{
		if (pData) n_delete_array(pData);
		pData = (NewSize > 0) ? n_new_array(T, NewSize) : nullptr;
	}

	Count = NewSize;
}
//---------------------------------------------------------------------

template<typename T, typename S> void CFixedArray<T, S>::Copy(const CFixedArray<T, S>& Other)
{
	if (this == &Other) return;
	SetSize(Other.Count);
	for (S i = 0; i < Count; ++i) pData[i] = Other.pData[i];
}
//---------------------------------------------------------------------

// NB: Doesn't call element constructors
template<typename T, typename S> void CFixedArray<T, S>::RawCopyFrom(const T* pSrc, S SrcCount)
{
	if (!pSrc) return;
	SetSize(SrcCount);
	if (SrcCount) memcpy(pData, pSrc, SrcCount * sizeof(T));
}
//---------------------------------------------------------------------

template<typename T, typename S> IPTR CFixedArray<T, S>::FindIndex(const T& Elm) const
{
	for (S i = 0; i < Count; ++i)
		if (Elm == pData[i]) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

template<typename T, typename S> inline T& CFixedArray<T, S>::operator [](S Index) const
{
	n_assert_dbg(pData && Index < Count);
	return pData[Index];
}
//---------------------------------------------------------------------

#endif
