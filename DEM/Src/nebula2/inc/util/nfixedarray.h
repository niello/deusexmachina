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

template<class TYPE> class nFixedArray
{
private:

	TYPE*	pData;
	int		Count;

	void	Allocate(int NewSize);
	void	Copy(const nFixedArray<TYPE>& src);
	void	Delete();

public:

	nFixedArray(): Count(0), pData(NULL) {}
	nFixedArray(int _Size): Count(0), pData(NULL) { Allocate(_Size); }
	nFixedArray(const nFixedArray<TYPE>& Other): Count(0), pData(NULL) { Copy(Other); }
	~nFixedArray();

	void	Clear(TYPE Elm) { for (int i = 0; i < Count; i++) pData[i] = Elm; }
	int		Find(const TYPE& Elm) const;

	void	SetSize(int NewSize) { if (Count != NewSize) Allocate(NewSize); }
	int		GetCount() const { return Count; }
	TYPE*	GetPtr() { return pData; }

	void	RawCopyFrom(const TYPE* pSrc, uint SrcCount);

	void	operator =(const nFixedArray<TYPE>& Other) { Copy(Other); }
	TYPE&	operator [](int Index) const;
};

template<class TYPE> nFixedArray<TYPE>::~nFixedArray()
{
	if (pData)
	{
		n_delete_array(pData);
		pData = NULL;
	}
}
//---------------------------------------------------------------------

template<class TYPE> void nFixedArray<TYPE>::Allocate(int NewSize)
{
	Delete();
	if (NewSize > 0)
	{
		pData = n_new_array(TYPE, NewSize);
		Count = NewSize;
	}
}
//---------------------------------------------------------------------

template<class TYPE> void nFixedArray<TYPE>::Copy(const nFixedArray<TYPE>& rhs)
{
	if (this != &rhs)
	{
		Allocate(rhs.Count);
		for (int i = 0; i < Count; i++) pData[i] = rhs.pData[i];
	}
}
//---------------------------------------------------------------------

template<class TYPE> void nFixedArray<TYPE>::Delete()
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
template<class TYPE> void nFixedArray<TYPE>::RawCopyFrom(const TYPE* pSrc, uint SrcCount)
{
	n_assert(pSrc);
	memcpy(pData, pSrc, n_min(SrcCount, Count));
}
//---------------------------------------------------------------------

template<class TYPE> TYPE& nFixedArray<TYPE>::operator [](int Index) const
{
	n_assert(pData && Index >= 0 && Index < Count);
	return pData[Index];
}
//---------------------------------------------------------------------

template<class TYPE> int nFixedArray<TYPE>::Find(const TYPE& Elm) const
{
	for (int i = 0; i < Count; i++)
		if (Elm == pData[i]) return i;
	return -1;
}
//---------------------------------------------------------------------

#endif
