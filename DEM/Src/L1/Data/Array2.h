#pragma once
#ifndef __DEM_L1_ARRAY2_H__
#define __DEM_L1_ARRAY2_H__

#include <StdDEM.h>

// Fixed size two-dimensional array.

template<class TYPE>
class CArray2
{
protected:

	TYPE*	pData;
	UPTR	Width;
	UPTR	Height;

	void	Alloc(UPTR W, UPTR H);
	void	Copy(const CArray2<TYPE>& src);
	void	Delete();

public:

	CArray2(): pData(NULL), Width(0), Height(0) {}
	CArray2(UPTR W, UPTR H): pData(NULL) { Alloc(W, H); }
	~CArray2() { Delete(); }

	void	SetSize(UPTR W, UPTR H) { Delete(); if (W > 0 && H > 0) Alloc(W, H); }

	TYPE&	At(UPTR Col, UPTR Row) const;
	void	Set(UPTR Col, UPTR Row, const TYPE& Elm);
	void	Clear(const TYPE& Elm) { for (UPTR i = 0; i < Width * Height; ++i) pData[i] = Elm; }

	UPTR	GetWidth() const { return Width; }
	UPTR	GetHeight() const { return Height; }

	CArray2<TYPE>& operator =(const CArray2<TYPE>& rhs);
};

template<class TYPE> void CArray2<TYPE>::Alloc(UPTR W, UPTR H)
{
	n_assert(!pData && W > 0 && H > 0);
	Width = W;
	Height = H;
	pData = n_new_array(TYPE, W * H);
}
//---------------------------------------------------------------------

template<class TYPE> void CArray2<TYPE>::Copy(const CArray2<TYPE>& src)
{
	Alloc(src.Width, src.Height);
	for (UPTR i = 0; i < Width * Height; ++i)
		pData[i] = src.pData[i];
}
//---------------------------------------------------------------------

template<class TYPE> void CArray2<TYPE>::Delete()
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

template<class TYPE> CArray2<TYPE>& CArray2<TYPE>::operator =(const CArray2<TYPE>& rhs)
{
	Delete();
	Copy(rhs);
	return *this;
}
//---------------------------------------------------------------------

template<class TYPE> TYPE& CArray2<TYPE>::At(UPTR Col, UPTR Row) const
{
	n_assert(pData && Col < Width && Row < Height);
	return pData[Row * Width + Col];
}
//---------------------------------------------------------------------

template<class TYPE> void CArray2<TYPE>::Set(UPTR Col, UPTR Row, const TYPE& Elm)
{
	n_assert(pData && Col < Width && Row < Height);
	pData[Row * Width + Col] = Elm;
}
//---------------------------------------------------------------------

#endif
