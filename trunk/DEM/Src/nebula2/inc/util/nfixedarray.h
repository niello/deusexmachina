#ifndef N_FIXEDARRAY_H
#define N_FIXEDARRAY_H
//------------------------------------------------------------------------------
/**
    @class nFixedArray
    @ingroup NebulaDataTypes

    @brief A fixed size, bounds checked array.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

template<class TYPE> class nFixedArray
{
private:

	int		size;
	TYPE*	pData;

	void	Allocate(int NewSize);
	void	Copy(const nFixedArray<TYPE>& src);
	void	Delete();

public:

	nFixedArray(): size(0), pData(NULL) {}
	nFixedArray(int _Size): size(0), pData(NULL) { Allocate(_Size); }
	nFixedArray(const nFixedArray<TYPE>& Other): size(0), pData(NULL) { Copy(Other); }
	~nFixedArray();

	void	Clear(TYPE Elm) { for (int i = 0; i < size; i++) pData[i] = Elm; }
	int		Find(const TYPE& Elm) const;

	void	SetSize(int NewSize) { if (size != NewSize) Allocate(NewSize); }
	int		Size() const { return size; }

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
		size = NewSize;
	}
}
//---------------------------------------------------------------------

template<class TYPE> void nFixedArray<TYPE>::Copy(const nFixedArray<TYPE>& rhs)
{
	if (this != &rhs)
	{
		Allocate(rhs.size);
		for (int i = 0; i < size; i++) pData[i] = rhs.pData[i];
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
	size = 0;
}
//---------------------------------------------------------------------

template<class TYPE> TYPE& nFixedArray<TYPE>::operator[](int Index) const
{
	n_assert(pData && Index >= 0 && Index < size);
	return pData[Index];
}
//---------------------------------------------------------------------

template<class TYPE> int nFixedArray<TYPE>::Find(const TYPE& Elm) const
{
	for (int i = 0; i < size; i++)
		if (Elm == pData[i]) return i;
	return -1;
}
//---------------------------------------------------------------------

#endif
