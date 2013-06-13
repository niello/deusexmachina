#ifndef N_FIXEDARRAY_H
#define N_FIXEDARRAY_H
//------------------------------------------------------------------------------
/**
    @class nFixedArray
    @ingroup NebulaDataTypes

    @brief A fixed Count, bounds checked array.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include <algorithm> // std::sort

template<class T> class nFixedArray
{
private:

	T*	pData;
	int		Count;

	void	Allocate(int NewSize);
	void	Copy(const nFixedArray<T>& src);
	void	Delete();

public:

	nFixedArray(): Count(0), pData(NULL) {}
	nFixedArray(int _Size): Count(0), pData(NULL) { Allocate(_Size); }
	nFixedArray(const nFixedArray<T>& Other): Count(0), pData(NULL) { Copy(Other); }
	~nFixedArray();

	void	Clear(T Elm) { for (int i = 0; i < Count; i++) pData[i] = Elm; }
	int		FindIndex(const T& Elm) const;
	bool	Contains(const T& Elm) const { return FindIndex(Elm) != -1; }
	void	Sort() { std::sort(pData, pData + Count); }
	template<class TCmp>
	void	Sort() { std::sort(pData, pData + Count, TCmp()); }

	void	SetSize(int NewSize) { if (Count != NewSize) Allocate(NewSize); }
	int		GetCount() const { return Count; }
	T*		GetPtr() { return pData; }

	void	RawCopyFrom(const T* pSrc, uint SrcCount);

	void	operator =(const nFixedArray<T>& Other) { Copy(Other); }
	T&	operator [](int Index) const;
};

template<class T> nFixedArray<T>::~nFixedArray()
{
	if (pData)
	{
		n_delete_array(pData);
		pData = NULL;
	}
}
//---------------------------------------------------------------------

template<class T> void nFixedArray<T>::Allocate(int NewSize)
{
	Delete();
	if (NewSize > 0)
	{
		pData = n_new_array(T, NewSize);
		Count = NewSize;
	}
}
//---------------------------------------------------------------------

template<class T> void nFixedArray<T>::Copy(const nFixedArray<T>& rhs)
{
	if (this != &rhs)
	{
		Allocate(rhs.Count);
		for (int i = 0; i < Count; i++) pData[i] = rhs.pData[i];
	}
}
//---------------------------------------------------------------------

template<class T> void nFixedArray<T>::Delete()
{
	if (pData)
	{
		n_delete_array(pData);
		pData = NULL;
	}
	Count = 0;
}
//---------------------------------------------------------------------

// Doesn't call element constructors
template<class T> void nFixedArray<T>::RawCopyFrom(const T* pSrc, uint SrcCount)
{
	n_assert(pSrc);
	memcpy(pData, pSrc, n_min(SrcCount, Count));
}
//---------------------------------------------------------------------

template<class T> T& nFixedArray<T>::operator [](int Index) const
{
	n_assert(pData && Index >= 0 && Index < Count);
	return pData[Index];
}
//---------------------------------------------------------------------

template<class T> int nFixedArray<T>::FindIndex(const T& Elm) const
{
	for (int i = 0; i < Count; i++)
		if (Elm == pData[i]) return i;
	return -1;
}
//---------------------------------------------------------------------

#endif
