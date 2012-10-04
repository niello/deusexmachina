#ifndef N_ARRAY2_H
#define N_ARRAY2_H

#include "kernel/ntypes.h"

// Fixed size two-dimensional array.

template<class TYPE>
class nArray2
{
protected:

	TYPE*	pData;
	uint	Width;
	uint	Height;

	void	Alloc(uint W, uint H);
	void	Copy(const nArray2<TYPE>& src);
	void	Delete();

public:

	nArray2(): pData(NULL), Width(0), Height(0) {}
	nArray2(uint W, uint H): pData(NULL) { Alloc(W, H); }
	~nArray2() { Delete(); }

	void	SetSize(uint W, uint H) { Delete(); if (W > 0 && H > 0) Alloc(W, H); }

	TYPE&	At(uint Col, uint Row) const;
	void	Set(uint Col, uint Row, const TYPE& Elm);
	void	Clear(const TYPE& Elm);

	uint	GetWidth() const { return Width; }
	uint	GetHeight() const { return Height; }

	nArray2<TYPE>& operator =(const nArray2<TYPE>& rhs);
};

template<class TYPE> void nArray2<TYPE>::Alloc(uint W, uint H)
{
	n_assert(!pData && W > 0 && H > 0);
	Width = W;
	Height = H;
	pData = n_new_array(TYPE, W * H);
}
//---------------------------------------------------------------------

template<class TYPE> void nArray2<TYPE>::Copy(const nArray2<TYPE>& src)
{
	Alloc(src.Width, src.Height);
	for (uint i = 0; i < Width * Height; i++)
		pData[i] = src.pData[i];
}
//---------------------------------------------------------------------

template<class TYPE> void nArray2<TYPE>::Delete()
{
	Width = 0;
	Height = 0;
	if (pData)
	{
		n_delete_array(pData);
		pData = NULL;
	}
}
//---------------------------------------------------------------------

template<class TYPE> nArray2<TYPE>& nArray2<TYPE>::operator =(const nArray2<TYPE>& rhs)
{
	Delete();
	Copy(rhs);
	return *this;
}
//---------------------------------------------------------------------

template<class TYPE> TYPE& nArray2<TYPE>::At(uint Col, uint Row) const
{
	n_assert(pData && Col < Width && Row < Height);
	return pData[Row * Width + Col];
}
//---------------------------------------------------------------------

template<class TYPE> void nArray2<TYPE>::Set(uint Col, uint Row, const TYPE& Elm)
{
	n_assert(pData && Col < Width && Row < Height);
	pData[Row * Width + Col] = Elm;
}
//---------------------------------------------------------------------

template<class TYPE> void nArray2<TYPE>::Clear(const TYPE& Elm)
{
	for (uint i = 0; i < Width * Height; i++)
		pData[i] = Elm;
}
//---------------------------------------------------------------------

#endif
