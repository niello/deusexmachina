#ifndef N_ARRAY_H
#define N_ARRAY_H
//------------------------------------------------------------------------------
/**
    @class nArray
    @ingroup NebulaDataTypes

    @brief A dynamic array template class, similar to the stl vector class.

    Can also be set to a fixed size (SetFixedSize()) if the size of
    the array is known beforehand. This eliminates the memory overhead
    for pre-allocated pData if the array works in dynamic mode. To
    prevent the array from pre-allocating any memory on construction
    call the nArray(0, 0) constructor.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include <Data/Flags.h>

#include <algorithm> // std::sort

enum
{
	Array_DoubleGrowSize	= 0x01,
	Array_KeepOrder			= 0x02	// Now is set by default //???clear by default?
};

template<class T> class nArray
{
private:

	T*				pData;
	int				Allocated;
	int				Count;
	int				GrowSize;

	void	MakeIndexValid(int Idx);
	void	Construct(T* pElm) { n_placement_new(pElm, T); }
	void	Construct(T* pElm, const T& Value) { n_placement_new(pElm, T)(Value); }
	void	Copy(const nArray<T>& src);
	void	Delete();
	void	Grow();
	void	Move(int fromIndex, int toIndex);
	void	MoveQuick(int fromIndex, int toIndex);

public:

	typedef T* CIterator;

	Data::CFlags	Flags; // nDictionary needs to access it

	nArray(): GrowSize(16), Allocated(0), Count(0), Flags(Array_KeepOrder), pData(NULL) {}
	nArray(int _Count, int _GrowSize);
	nArray(int _Count, int _GrowSize, const T& Value);
	nArray(const nArray<T>& Other): GrowSize(0), Allocated(0), Count(0), pData(0), Flags(Array_KeepOrder) { Copy(Other); }
	~nArray() { Delete(); }

	T&			Append(const T& pElm);
	void		AppendArray(const nArray<T>& Other);
	CIterator	Reserve(int num, bool Grow = true);
	T&			Set(int Idx, const T& pElm);
	T&			Insert(int Idx, const T& pElm);
	T&			InsertSorted(const T& pElm);
	void		Fill(int first, int num, const T& pElm);

	T&			At(int Idx);
	T&			At(int Idx) const;
	T&			Front() const { n_assert(pData && Count > 0); return pData[0]; }
	T&			Back() const { n_assert(pData && Count > 0); return pData[Count - 1]; }
	CIterator	Begin() const { return pData; }
	CIterator	End() const { return pData + Count; }

	bool		RemoveByValue(const T& Elm);
	void		EraseAt(int Idx);
	void		EraseAtQuick(int Idx);
	CIterator	Erase(CIterator iter);
	CIterator	EraseQuick(CIterator iter);
	void		Clear();
	void		Reset() { Count = 0; }

	void		Resize(int NewAllocSize);
	void		Reallocate(int _Count, int _GrowSize);

	CIterator	Find(const T& Elm) const;
	int			FindIndex(const T& Elm) const;
	bool		Contains(const T& Elm) const { return FindIndex(Elm) != -1; }
	int			BinarySearchIndex(const T& pElm) const;
	void		Sort() { std::sort(Begin(), End()); }
	template<class TCmp>
	void		Sort() { std::sort(Begin(), End(), TCmp()); }
	nArray<T>	Difference(const nArray<T>& Other) const;

	int			GetCount() const { return Count; }
	int			GetAllocSize() const { return Allocated; }
	bool		IsEmpty() const { return !Count; }
	void		SetFixedSize(int Size); //???need to clear all content?
	void		SetGrowSize(int Grow) { GrowSize = Grow; }
	void		SetDoubleGrow(bool Double) { Flags.SetTo(Array_DoubleGrowSize, Double); }
	void		SetKeepOrder(bool Keep) { Flags.SetTo(Array_KeepOrder, Keep); }
	bool		IsDoubleGrowing() const { return Flags.Is(Array_DoubleGrowSize); }
	bool		IsKeepingOrder() const { return Flags.Is(Array_KeepOrder); }

	nArray<T>&	operator =(const nArray<T>& Other) { if (this != &Other) Copy(Other); return *this; }
	T&			operator [](int Idx) const { n_assert(Idx >= 0 && Idx < Count); return pData[Idx]; }
	bool		operator ==(const nArray<T>& Other) const;
	bool		operator !=(const nArray<T>& Other) const { return !(*this == Other); }
};

// NB: '_GrowSize' can be zero to create a static preallocated array.
template<class T>
nArray<T>::nArray(int _Count, int _GrowSize):
	GrowSize(_GrowSize),
	Allocated(_Count),
	Count(0),
	Flags(Array_KeepOrder)
{
	n_assert(_Count >= 0);
	pData = (_Count > 0) ? (T*)n_malloc(sizeof(T) * this->Allocated) : NULL;
}
//---------------------------------------------------------------------

template<class T>
nArray<T>::nArray(int _Count, int _GrowSize, const T& Value):
	GrowSize(_GrowSize),
	Allocated(_Count),
	Count(_Count),
	Flags(Array_KeepOrder)
{
	n_assert(_Count >= 0);
	if (_Count > 0)
	{
		pData = (T*)n_malloc(sizeof(T) * Allocated);
		for (int i = 0; i < _Count; i++)
			Construct(pData + i, Value);
	}
	else pData = NULL;
}
//---------------------------------------------------------------------

template<class T>
void nArray<T>::Copy(const nArray<T>& Other)
{
	if (Allocated < Other.Allocated)
	{
		Delete();
		pData = (T*)n_malloc(sizeof(T) * Other.Allocated);
	}
	else for (int i = 0; i < Count; ++i) pData[i].~T();

	for (int i = 0; i < Other.Count; ++i)
		Construct(pData + i, Other.pData[i]);

	Flags = Other.Flags;
	GrowSize = Other.GrowSize;
	Allocated = Other.Allocated;
	Count = Other.Count;
}
//---------------------------------------------------------------------

template<class T>
void nArray<T>::Delete()
{
	if (pData)
	{
		for (int i = 0; i < Count; i++)
			pData[i].~T();
		n_free(pData);
		pData = NULL;
	}
	Allocated = 0;
	Count = 0;
}
//---------------------------------------------------------------------

template<class T>
void nArray<T>::Reallocate(int _Count, int _GrowSize)
{
	Delete();
	GrowSize = _GrowSize;
	Allocated = _Count;
	Count = 0;
	pData = (_Count > 0) ? (T*)n_malloc(sizeof(T) * _Count) : 0;
}
//---------------------------------------------------------------------

// Set a new fixed size. This will throw away the current content, and
// create preallocate a new array which cannot _GrowSize. All pData in
// the array will be valid.
template<class T>
void nArray<T>::SetFixedSize(int size)
{
	Reallocate(size, 0);
	Count = size;
	for (int i = 0; i < size; i++)
		Construct(pData + i);
}
//---------------------------------------------------------------------

template<class T>
void nArray<T>::Resize(int NewAllocSize)
{
	if (Allocated == NewAllocSize) return;

	for (int i = NewAllocSize; i < Count; ++i)
		pData[i].~T();

	pData = (T*)n_realloc(pData, sizeof(T) * NewAllocSize);
	n_assert_dbg(!NewAllocSize || pData);

	Allocated = NewAllocSize;
	Count = (NewAllocSize < Count) ? NewAllocSize : Count;

	/*
	T* pNewData = (T*)n_malloc(sizeof(T) * NewAllocSize);

	int NewSize = (NewAllocSize < Count) ? NewAllocSize : Count;

	if (pData)
	{
		for (int i = 0; i < NewSize; i++)
		{
			Construct(pNewData + i, pData[i]);
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
void nArray<T>::Grow()
{
	n_assert(GrowSize > 0);
	Resize(Flags.Is(Array_DoubleGrowSize) ? (Allocated ? (Allocated << 1) : GrowSize) : Allocated + GrowSize);
}
//---------------------------------------------------------------------

/**
     - 30-Jan-03   floh    serious bugfixes!
     - 07-Dec-04   jo      bugfix: neededSize >= this->Allocated becomes
                                   neededSize > Allocated
*/
template<class T>
void nArray<T>::Move(int fromIndex, int toIndex)
{
    n_assert(pData);
    n_assert(fromIndex < this->Count);

    // nothing to move?
    if (fromIndex == toIndex) return;

    // compute number of pData to move
    int num = this->Count - fromIndex;

    // check if array needs to _GrowSize
	//!!!OPTIMIZE IT!
    int neededSize = toIndex + num;
    while (neededSize > this->Allocated) Grow();

    if (fromIndex > toIndex)
    {
        // this is a backward move
        // create front pData first
        int createCount = fromIndex - toIndex;
        T* from = pData + fromIndex;
        T* to = pData + toIndex;
        for (int i = 0; i < createCount; i++)
			Construct(to + i, from[i]);

        // copy remaining pData
        int copyCount = num - createCount;
        from = pData + fromIndex + createCount;
        to = pData + toIndex + createCount;
        for (int i = 0; i < copyCount; i++) 
            to[i] = from[i];

        // destroy remaining pData
        for (int i = toIndex + num; i < this->Count; i++)
			pData[i].~T();
    }
    else
    {
        // this is a forward move
        // create front pData first
        int createCount = toIndex - fromIndex;
        T* from = pData + fromIndex + num - createCount;
        T* to = pData + fromIndex + num;
        for (int i = 0; i < createCount; i++)
        {
            this->Construct(to + i, from[i]);
        }

        // copy remaining pData, this time backward copy
        int copyCount = num - createCount;
        from = (T*)pData + fromIndex;
        to = (T*)pData + toIndex;
        for (int i = copyCount - 1; i >= 0; i--) 
            to[i] = from[i];

        // destroy freed pData
        for (int i = fromIndex; i < toIndex; i++)
            pData[i].~T();

        // be aware of these uninitialized element slots
        // as far as I known, this part is only used in nArray::Insert
        // and it will fill in the blank element slot
    }

    // adjust array size
    this->Count = toIndex + num;
}

//------------------------------------------------------------------------------
/**
    Very fast move which does not call assignment operators or destructors,
    so you better know what you do!
*/
template<class T>
void nArray<T>::MoveQuick(int fromIndex, int toIndex)
{
	if (fromIndex == toIndex) return;
	n_assert(pData);
	n_assert(fromIndex < Count);
	int num = Count - fromIndex;
	memmove(pData + toIndex, pData + fromIndex, sizeof(T) * num);
	Count = toIndex + num;
}
//---------------------------------------------------------------------

template<class T>
T& nArray<T>::Append(const T& pElm)
{
	if (Count == Allocated) Grow();
	n_assert(pData);
	Construct(pData + Count, pElm);
	return pData[Count++];
}
//---------------------------------------------------------------------

template<class T>
void nArray<T>::AppendArray(const nArray<T>& Other)
{
	if (Count + Other.Count > Allocated)
		Resize(Count + Other.Count);
	for (int i = 0; i < Other.Count; i++)
		Append(Other[i]);
}
//---------------------------------------------------------------------

// Make room for N new pData at the end of the array, and return a pointer
// to the start of the reserved area. This can be (carefully!) used as a fast
// shortcut to fill the array directly with data.
template<class T>
typename nArray<T>::CIterator nArray<T>::Reserve(int num, bool Grow)
{
	n_assert(num > 0);
	int maxElement = Count + num;
	if (maxElement > Allocated)
	{
		if (Grow && num < GrowSize) Resize(Count + GrowSize);
		else Resize(maxElement);
	}
	n_assert(pData);
	CIterator iter = pData + Count;
	for (int i = Count; i < maxElement; i++)
		Construct(pData + i);
	Count = maxElement;
	return iter;
}
//---------------------------------------------------------------------

// This will check if the provided Idx is in the valid range. If it is
// not the array will be grown to that Idx.
template<class T>
void nArray<T>::MakeIndexValid(int Idx)
{
	if (Idx < Count) return;
	if (Idx >= Allocated)
	{
		n_assert(GrowSize > 0);
		Resize(Idx + GrowSize);
	}
	n_assert(Idx < Allocated);
	for (int i = Count; i <= Idx; i++) Construct(pData + i);
	Count = Idx + 1;
}
//---------------------------------------------------------------------

template<class T>
T& nArray<T>::Set(int Idx, const T& pElm)
{
	MakeIndexValid(Idx);
	pData[Idx] = pElm;
	return pData[Idx];
}
//---------------------------------------------------------------------

// Access an element. This method may grow the array if the Idx is outside the array range.
template<class T> inline T& nArray<T>::At(int Idx)
{
	MakeIndexValid(Idx);
	return pData[Idx];
}
//---------------------------------------------------------------------

// Access an element. This method may grow the array if the Idx is outside the array range.
template<class T> inline T& nArray<T>::At(int Idx) const
{
	n_assert(Idx >= 0 && Idx < Count);
	return pData[Idx];
}
//---------------------------------------------------------------------

template<class T>
bool nArray<T>::operator ==(const nArray<T>& Other) const
{
	if (Other.Count != Count) return false;
	for (int i = 0; i < Count; i++)
		if (pData[i] != Other.pData[i])
			return false;
	return true;
}
//---------------------------------------------------------------------

template<class T>
bool nArray<T>::RemoveByValue(const T& pElm)
{
	int Idx = FindIndex(pElm);
	if (Idx == -1) return false;
	EraseAt(Idx);
	return true;
}
//---------------------------------------------------------------------

template<class T>
void nArray<T>::EraseAt(int Idx)
{
	n_assert(pData && Idx >= 0 && Idx < Count);
	pData[Idx].~T();
	if (Idx == Count - 1) --Count;
	else if (Flags.Is(Array_KeepOrder)) Move(Idx + 1, Idx);
	else
	{
		//???or use memcpy?
		Construct(pData + Idx, pData[Count - 1]);
		pData[Count - 1].~T();
		--Count;
	}
}
//---------------------------------------------------------------------

// Quick erase, uses memmove() and does not call assignment operators
// or destructor, so be careful about that!
template<class T>
inline void nArray<T>::EraseAtQuick(int Idx)
{
	n_assert(pData && Idx >= 0 && Idx < Count);
	if (Idx == Count - 1) Count--;
	else if (Flags.Is(Array_KeepOrder)) MoveQuick(Idx + 1, Idx);
	else
	{
		memcpy(pData + Idx, pData + Count - 1, sizeof(T));
		--Count;
	}
}
//---------------------------------------------------------------------

template<class T>
inline typename nArray<T>::CIterator
nArray<T>::Erase(typename nArray<T>::CIterator It)
{
	if (It) EraseAt(int(It - pData));
	return It;
}
//---------------------------------------------------------------------

// Quick erase, uses memmove() and does not call assignment operators
// or destructor, so be careful about that!
template<class T>
inline typename nArray<T>::CIterator
nArray<T>::EraseQuick(typename nArray<T>::CIterator iter)
{
	n_assert(pData && iter >= pData && iter < pData + Count);
	EraseQuick(int(iter - pData));
	return iter;
}
//---------------------------------------------------------------------

template<class T>
T& nArray<T>::Insert(int Idx, const T& pElm)
{
	n_assert(Idx >= 0 && Idx <= Count);
	if (Idx == Count) return Append(pElm);
	else
	{
		Move(Idx, Idx + 1);
		Construct(pData + Idx, pElm);
		return pData[Idx];
	}
}
//---------------------------------------------------------------------

template<class T>
void nArray<T>::Clear()
{
	for (int i = 0; i < Count; i++) pData[i].~T();
	Count = 0;
}
//---------------------------------------------------------------------

template<class T>
typename nArray<T>::CIterator nArray<T>::Find(const T& pElm) const
{
	for (int i = 0; i < Count; ++i)
		if (pData[i] == pElm)
			return pData + i;
	return NULL;
}
//---------------------------------------------------------------------

template<class T>
int nArray<T>::FindIndex(const T& pElm) const
{
	for (int i = 0; i < Count; ++i)
		if (pData[i] == pElm)
			return i;
	return -1;
}
//------------------------------------------------------------------------------

template<class T>
void nArray<T>::Fill(int First, int Num, const T& pElm)
{
	n_assert(First >= 0 && First <= Count && Num >= 0);

	int End = First + Num;
	if (End > Count)
	{
		if (End > Allocated) Resize(End);
		// fill the tailing pData
		for (int i = Count; i < End; i++)
			Construct(pData + i, pElm);
		// the rest
		End = Count;
		Count = First + Num;
	}

	for (int i = First; i < End; i++)
	{
		pData[i].~T();
		Construct(pData + i, pElm);
	}
}

//------------------------------------------------------------------------------

// Returns a new array with all element which are in Other, but not in this.
// Be careful, this method may be very slow with large arrays!
template<class T>
nArray<T> nArray<T>::Difference(const nArray<T>& Other) const
{
	nArray<T> diff;
	for (int i = 0; i < Other.Count; i++)
		if (!Find(Other[i])) diff.Append(Other[i]);
	return diff;
}
//---------------------------------------------------------------------

/**
    Does a binary search on the array, returns the Idx of the identical
    element, or -1 if not found
*/
template<class T>
int nArray<T>::BinarySearchIndex(const T& pElm) const
{
	if (!Count) return -1;

	int num = Count, lo = 0, hi = num - 1;
	while (lo <= hi)
	{
		int half = (num >> 1);
		if (half)
		{
			int mid = lo + half - 1 + (num & 1);
			if (pElm < pData[mid])
			{
				hi = mid - 1;
				num = mid - lo;
			}
			else if (pElm > pData[mid])
			{
				lo = mid + 1;
				num = half;
			}
			else return mid;
		}
		else return (num && pElm == pData[lo]) ? lo : -1;
	}

	return -1;
}

//------------------------------------------------------------------------------
/**
    This inserts the element into a sorted array. In the current
    implementation this is a slow operation O(n). This should be
    optimized to O(log n).
*/
template<class T>
T& nArray<T>::InsertSorted(const T& pElm)
{
	for (int i = 0; i < Count; i++)
		if (pElm < pData[i])
			return Insert(i, pElm);
	return Append(pElm);
}
//---------------------------------------------------------------------

#endif
