#pragma once
#ifndef __DEM_L1_ARRAY_H__
#define __DEM_L1_ARRAY_H__

#include <Data/Flags.h>
#include <System/System.h>
#include <Data/ArrayUtils.h>	// Search and closest index
#include <algorithm>			// std::sort

// A dynamic array template class

enum
{
	Array_DoubleGrowSize	= 0x01,
	Array_KeepOrder			= 0x02	// Now is set by default //???clear by default?
};

//???template parameter bool UseConstructorsAndDestructors? Or IsPOD<T>()?
template<class T>
class CArray
{
private:

	T*		pData;
	UPTR	Allocated;
	UPTR	Count;
	UPTR	GrowSize;

	void	MakeIndexValid(UPTR Idx);
	UPTR	GetActualGrowSize() { return (Flags.Is(Array_DoubleGrowSize) && Allocated) ? Allocated : GrowSize; }
	void	Grow();
	void	GrowTo(UPTR NewCount);
	void	Move(IPTR FromIdx, IPTR ToIdx);

public:

	typedef T* CIterator;

	Data::CFlags	Flags; // CDict needs to access it

	CArray(): pData(nullptr), Allocated(0), Count(0), GrowSize(16), Flags(Array_KeepOrder) {}
	CArray(UPTR _Count, UPTR _GrowSize);
	CArray(UPTR _Count, UPTR _GrowSize, const T& Value);
	CArray(const CArray<T>& Other): pData(nullptr), Allocated(0), Count(0) { Copy(Other); }
	CArray(CArray<T>&& Other);
	~CArray();

	CIterator	Add();
	CIterator	Add(const T& Val);
	CIterator	AddBefore(CIterator It, const T& Val) { return Insert(It ? IndexOf(It) : 0, Val); }
	CIterator	AddAfter(CIterator It, const T& Val) { return Insert(It ? IndexOf(It) + 1 : 0, Val); }
	CIterator	Reserve(UPTR Num, bool Grow = true);
	CIterator	Insert(IPTR Idx, const T& Val);
	CIterator	InsertSorted(const T& Val) { return Insert(FindClosestIndexSorted(Val), Val); }
	void		AddArray(const CArray<T>& Other);
	void		AddArray(const T* pData, UPTR Size);
	void		Copy(const CArray<T>& Other);
	void		Fill(UPTR First, UPTR Num, const T& Val);
	void		AllocateFixed(UPTR NewCount);

	void		Remove(CIterator It, T* pOutValue = nullptr) { n_assert_dbg(It); RemoveAt(IPTR(It - pData), pOutValue); }
	void		RemoveAt(IPTR Idx, T* pOutValue = nullptr);
	bool		RemoveByValue(const T& Val);
	bool		RemoveByValueSorted(const T& Val);
	void		Clear(bool FreeMemory = false);

	T&			Front() const { n_assert(pData && Count > 0); return pData[0]; }
	T&			Back() const { n_assert(pData && Count > 0); return pData[Count - 1]; }
	CIterator	Begin() const { return pData; }
	CIterator	End() const { return pData + Count; }
	T&			At(IPTR Idx) { MakeIndexValid(Idx); return pData[Idx]; }
	T&			At(IPTR Idx) const { n_assert(IsIndexValid(Idx)); return pData[Idx];}
	CIterator	IteratorAt(IPTR Idx) { MakeIndexValid(Idx); return pData + Idx; }
	CIterator	IteratorAt(IPTR Idx) const { return Idx == INVALID_INDEX ? nullptr : pData + Idx; }
	IPTR		IndexOf(CIterator It) const { return It - pData; }

	CIterator	Find(const T& Val) const { IPTR Idx = FindIndex(Val); return Idx == INVALID_INDEX ? End() : IteratorAt(Idx); }
	IPTR		FindIndex(const T& Val) const;
	CIterator	FindSorted(const T& Val) const { IPTR Idx = ArrayUtils::FindIndexSorted(pData, Count, Val); return Idx == INVALID_INDEX ? End() : IteratorAt(Idx); }
	IPTR		FindIndexSorted(const T& Val) const { return ArrayUtils::FindIndexSorted(pData, Count, Val); }
	CIterator	FindClosestSorted(const T& Val, bool* pHasEqualElement = nullptr) const { return IteratorAt(FindClosestIndexSorted(Val, pHasEqualElement)); }
	IPTR		FindClosestIndexSorted(const T& Val, bool* pHasEqualElement = nullptr) const { return ArrayUtils::FindClosestIndexSorted(pData, Count, Val, pHasEqualElement); }
	bool		Contains(const T& Val) const { return FindIndex(Val) != INVALID_INDEX; }
	bool		ContainsSorted(const T& Val) const { return ArrayUtils::FindIndexSorted(pData, Count, Val) != INVALID_INDEX; }
	bool		IsIndexValid(IPTR Idx) const { return ((UPTR)Idx) < Count; }

	void		Resize(UPTR NewAllocSize);
	void		Reallocate(UPTR NewAllocSize, UPTR NewGrowSize);
	void		Truncate(UPTR TailCount);

	void		Sort() { std::sort(Begin(), End()); }
	template<class TCmp>
	void		Sort() { std::sort(Begin(), End(), TCmp()); }
	UPTR		Difference(const CArray<T>& Other, CArray<T>& Out) const;

	bool		IsEmpty() const { return !Count; }
	UPTR		GetCount() const { return Count; }
	UPTR		GetAllocSize() const { return Allocated; }
	void		SetGrowSize(UPTR Grow) { GrowSize = Grow; }
	void		SetDoubleGrow(bool Double) { Flags.SetTo(Array_DoubleGrowSize, Double); }
	void		SetKeepOrder(bool Keep) { Flags.SetTo(Array_KeepOrder, Keep); }
	bool		IsDoubleGrowing() const { return Flags.Is(Array_DoubleGrowSize); }
	bool		IsKeepingOrder() const { return Flags.Is(Array_KeepOrder); }

	// Range-based 'for' loop support
	CIterator	begin() const { return pData; }
	CIterator	end() const { return pData + Count; }

	CArray<T>&	operator =(const CArray<T>& Other) { if (this != &Other) Copy(Other); return *this; }
	T&			operator [](IPTR Idx) const { return At(Idx); }
	bool		operator ==(const CArray<T>& Other) const;
	bool		operator !=(const CArray<T>& Other) const { return !(*this == Other); }
};

// NB: '_GrowSize' can be zero to create a static preallocated array.
template<class T>
CArray<T>::CArray(UPTR _Count, UPTR _GrowSize):
	GrowSize(_GrowSize),
	Allocated(_Count),
	Count(0),
	Flags(Array_KeepOrder)
{
	pData = (_Count > 0) ? (T*)n_malloc(sizeof(T) * Allocated) : nullptr;
}
//---------------------------------------------------------------------

template<class T>
CArray<T>::CArray(UPTR _Count, UPTR _GrowSize, const T& Value):
	GrowSize(_GrowSize),
	Allocated(_Count),
	Count(_Count),
	Flags(Array_KeepOrder)
{
	if (_Count > 0)
	{
		pData = (T*)n_malloc(sizeof(T) * Allocated);
		for (UPTR i = 0; i < _Count; ++i) n_placement_new(pData + i, T)(Value);
	}
	else pData = nullptr;
}
//---------------------------------------------------------------------

template<class T>
CArray<T>::CArray(CArray<T>&& Other)
	: pData(Other.pData)
	, Allocated(Other.Allocated)
	, Count(Other.Count)
	, GrowSize(Other.GrowSize)
	, Flags(Other.Flags)
{
	Other.pData = nullptr;
	Other.Allocated = 0;
	Other.Count = 0;
}
//---------------------------------------------------------------------

template<class T>
CArray<T>::~CArray()
{
	if (pData)
	{
		for (UPTR i = 0; i < Count; ++i) pData[i].~T();
		n_free(pData);
	}
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::Copy(const CArray<T>& Other)
{
	for (UPTR i = 0; i < Count; ++i) pData[i].~T();

	if (Allocated < Other.Allocated)
		pData = (T*)n_realloc(pData, sizeof(T) * Other.Allocated);

	for (UPTR i = 0; i < Other.Count; ++i)  n_placement_new(pData + i, T)(Other.pData[i]);

	Flags = Other.Flags;
	GrowSize = Other.GrowSize;
	Allocated = Other.Allocated;
	Count = Other.Count;
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::Fill(UPTR First, UPTR Num, const T& Val)
{
	n_assert(IsIndexValid(First));

	UPTR NewCount = First + Num;
	GrowTo(NewCount);

	UPTR i = First;
	for (; i < Count; ++i) pData[i] = Val;
	for (; i < NewCount; ++i) n_placement_new(pData + i, T)(Val);
	Count = NewCount;
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::AllocateFixed(UPTR NewCount)
{
	Reallocate(NewCount, 0);
	Count = NewCount;
	for (UPTR i = 0; i < Count; ++i) n_placement_new(pData + i, T);
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::Reallocate(UPTR NewAllocSize, UPTR NewGrowSize)
{
	for (UPTR i = 0; i < Count; ++i) pData[i].~T();
	GrowSize = NewGrowSize;
	Allocated = NewAllocSize;
	Count = 0;
	if (Allocated > 0) pData = (T*)n_realloc(pData, sizeof(T) * Allocated);
	else { SAFE_FREE(pData); }
}
//---------------------------------------------------------------------

// NB: this implementation can invoke raw memory copying through n_realloc,
// that doesn't call constructors and destructors, but changes object location.
template<class T>
void CArray<T>::Resize(UPTR NewAllocSize)
{
	if (Allocated == NewAllocSize) return;

	for (UPTR i = NewAllocSize; i < Count; ++i) pData[i].~T();

	pData = (T*)n_realloc(pData, sizeof(T) * NewAllocSize);
	n_assert_dbg(!NewAllocSize || pData);

	Allocated = NewAllocSize;
	Count = n_min(Allocated, Count);

	/*
	// NB: variant above doesn't construct objects above the Count! It is good, and Move() relies on it!

	T* pNewData = (T*)n_malloc(sizeof(T) * NewAllocSize);

	UPTR NewSize = (NewAllocSize < Count) ? NewAllocSize : Count;

	if (pData)
	{
		for (UPTR i = 0; i < NewSize; ++i)
		{
			Construct(pNewData + i, pData[i]); //n_placement_new(pData + i, T)(Val)
			pData[i].~T();
		}
		n_free(pData);
	}

	pData = pNewData;
	Allocated = NewAllocSize;
	Count = NewSize;
*/
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::Truncate(UPTR TailCount)
{
	UPTR NewCount = (TailCount > Count) ? 0 : Count - TailCount;
	for (UPTR i = NewCount; i < Count; ++i) pData[i].~T();
	Count = NewCount;
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::Grow()
{
	UPTR CurrGrow = GetActualGrowSize();
	n_assert2(CurrGrow, "Request to grow non-growable array");
	Resize(Allocated + CurrGrow);
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::GrowTo(UPTR NewCount)
{
	if (NewCount <= Allocated) return;
	UPTR NewAllocSize = Allocated;
	NewAllocSize += GetActualGrowSize();
	n_assert2(NewAllocSize > Allocated, "Request to grow non-growable array");
	while (NewCount > NewAllocSize) NewAllocSize += GetActualGrowSize();
	Resize(NewAllocSize);
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::Move(IPTR FromIdx, IPTR ToIdx)
{
	if (FromIdx == ToIdx) return;

	n_assert(IsIndexValid(FromIdx) && ToIdx >= 0);

	UPTR MoveCount = Count - FromIdx;

	// Fast way w/o constructors and destructors:
	//memmove(pData + ToIdx, pData + FromIdx, sizeof(T) * MoveCount);

	UPTR NewCount = ToIdx + MoveCount;
	GrowTo(NewCount);

	if (FromIdx > ToIdx) // Backward
	{
		T* pDataSrc = pData + FromIdx;
		T* pDataDest = pData + ToIdx;
		T* pDataLast = pData + Count;
		for (; pDataSrc < pDataLast; ++pDataSrc, ++pDataDest)
			*pDataDest = *pDataSrc;

		pDataSrc = pData + NewCount;
		for (; pDataSrc < pDataLast; ++pDataSrc) pDataSrc->~T();
	}
	else // Forward
	{
		// NB Nebula2: Be aware of uninitialized slots between FromIdx and ToIdx

		UPTR OldDataEndIdx = n_min(Count, ToIdx);

		T* pDataSrc = pData + Count - 1;
		T* pDataDest = pData + NewCount - 1;
		T* pDataLast = pDataSrc - OldDataEndIdx + FromIdx;
		for (; pDataSrc > pDataLast; --pDataSrc, --pDataDest)
			n_placement_new(pDataDest, T)(*pDataSrc);

		pDataLast = pData + FromIdx;
		for (; pDataSrc >= pDataLast; --pDataSrc, --pDataDest)
			*pDataDest = *pDataSrc;

		pDataSrc = pDataLast;
		pDataLast = pData + OldDataEndIdx;
		for (; pDataSrc < pDataLast; ++pDataSrc) pDataSrc->~T();
	}

	Count = NewCount;
}
//---------------------------------------------------------------------

template<class T>
typename CArray<T>::CIterator CArray<T>::Add()
{
	if (Count == Allocated) Grow();
	n_placement_new(pData + Count, T);
	++Count;
	return pData + Count - 1;
}
//---------------------------------------------------------------------

template<class T>
typename CArray<T>::CIterator CArray<T>::Add(const T& Val)
{
	if (Count == Allocated) Grow();
	n_placement_new(pData + Count, T)(Val);
	++Count;
	return pData + Count - 1;
}
//---------------------------------------------------------------------

template<class T>
typename CArray<T>::CIterator CArray<T>::Insert(IPTR Idx, const T& Val)
{
	n_assert2_dbg(Flags.Is(Array_KeepOrder), "Insertion has no much meaning if order isn't preserver, use Add(), it is faster!");
	n_assert(((UPTR)Idx) <= Count);
	if (Idx == Count) return Add(Val);
	else
	{
		Move(Idx, Idx + 1);
		n_placement_new(pData + Idx, T)(Val);
		return pData + Idx;
	}
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::AddArray(const CArray<T>& Other)
{
	UPTR NewCount = Count + Other.Count;
	if (NewCount > Allocated) Resize(NewCount);
	T* pEnd = pData + Count;
	for (UPTR i = 0; i < Other.Count; ++i)
		n_placement_new(pEnd + i, T)(Other.pData[i]);
	Count = NewCount;
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::AddArray(const T* pData, UPTR Size)
{
	if (Count + Size > Allocated)
		Resize(Count + Size);
	for (UPTR i = 0; i < Size; ++i)
		Add(pData[i]);
}
//---------------------------------------------------------------------

// Make room for N new data at the end of the array, and return a pointer
// to the start of the reserved area. This can be (carefully!) used as a fast
// shortcut to fill the array directly with data.
template<class T>
typename CArray<T>::CIterator CArray<T>::Reserve(UPTR Num, bool Grow)
{
	if (!Num) return nullptr;

	UPTR NewCount = Count + Num;
	if (NewCount > Allocated)
	{
		if (Grow) GrowTo(NewCount);
		else Resize(NewCount);
	}

	CIterator It = pData + Count;
	for (UPTR i = Count; i < NewCount; ++i) n_placement_new(pData + i, T);
	Count = NewCount;
	return It;
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::MakeIndexValid(UPTR Idx)
{
	if (Idx < Count) return;
	if (Idx >= Allocated)
	{
		n_assert2(GrowSize, "Request to grow non-growable array");
		Resize(Idx + GrowSize);
	}
	for (UPTR i = Count; i <= Idx; ++i) n_placement_new(pData + i, T);
	Count = Idx + 1;
}
//---------------------------------------------------------------------

template<class T>
bool CArray<T>::operator ==(const CArray<T>& Other) const
{
	if (Other.Count != Count) FAIL;
	for (UPTR i = 0; i < Count; ++i)
		if (pData[i] != Other.pData[i])
			FAIL;
	OK;
}
//---------------------------------------------------------------------

template<class T>
bool CArray<T>::RemoveByValue(const T& Val)
{
	IPTR Idx = FindIndex(Val);
	if (Idx == INVALID_INDEX) FAIL;
	RemoveAt(Idx);
	OK;
}
//---------------------------------------------------------------------

template<class T>
bool CArray<T>::RemoveByValueSorted(const T& Val)
{
	IPTR Idx = ArrayUtils::FindIndexSorted(pData, Count, Val);
	if (Idx == INVALID_INDEX) FAIL;
	RemoveAt(Idx);
	OK;
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::RemoveAt(IPTR Idx, T* pOutValue)
{
	n_assert(IsIndexValid(Idx));
	if (pOutValue) *pOutValue = pData[Idx];
	if (Idx == Count - 1)
	{
		--Count;
		pData[Count].~T();
	}
	else if (Flags.Is(Array_KeepOrder)) Move(Idx + 1, Idx);
	else
	{
		--Count;
		//???or use memcpy instead of operator =?
		pData[Idx] = pData[Count];
		pData[Count].~T();
	}
}
//---------------------------------------------------------------------

template<class T>
void CArray<T>::Clear(bool FreeMemory)
{
	for (UPTR i = 0; i < Count; ++i) pData[i].~T();
	Count = 0;
	if (FreeMemory && pData)
	{
		n_free(pData);
		pData = nullptr;
		Allocated = 0;
	}
}
//---------------------------------------------------------------------

template<class T>
IPTR CArray<T>::FindIndex(const T& Val) const
{
	IPTR Idx = &Val - pData;
	if (IsIndexValid(Idx)) return Idx;
	for (UPTR i = 0; i < Count; ++i)
		if (pData[i] == Val) return i;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------

// Returns a new array with all element which are in Other, but not in this.
// Be careful, this method may be very slow with large arrays!
template<class T>
UPTR CArray<T>::Difference(const CArray<T>& Other, CArray<T>& Out) const
{
	UPTR OutCount = Out.GetCount();
	for (UPTR i = 0; i < Other.Count; ++i)
		if (!Find(Other[i]))
			Out.Add(Other[i]);
	return Out.GetCount() - OutCount;
}
//---------------------------------------------------------------------

#endif
