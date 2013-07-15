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
	DWORD	Width;
	DWORD	Height;

	void	Alloc(DWORD W, DWORD H);
	void	Copy(const CArray2<TYPE>& src);
	void	Delete();

public:

	CArray2(): pData(NULL), Width(0), Height(0) {}
	CArray2(DWORD W, DWORD H): pData(NULL) { Alloc(W, H); }
	~CArray2() { Delete(); }

	void	SetSize(DWORD W, DWORD H) { Delete(); if (W > 0 && H > 0) Alloc(W, H); }

	TYPE&	At(DWORD Col, DWORD Row) const;
	void	Set(DWORD Col, DWORD Row, const TYPE& Elm);
	void	Clear(const TYPE& Elm) { for (DWORD i = 0; i < Width * Height; i++) pData[i] = Elm; }

	DWORD	GetWidth() const { return Width; }
	DWORD	GetHeight() const { return Height; }

	CArray2<TYPE>& operator =(const CArray2<TYPE>& rhs);
};

template<class TYPE> void CArray2<TYPE>::Alloc(DWORD W, DWORD H)
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
	for (DWORD i = 0; i < Width * Height; i++)
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

template<class TYPE> TYPE& CArray2<TYPE>::At(DWORD Col, DWORD Row) const
{
	n_assert(pData && Col < Width && Row < Height);
	return pData[Row * Width + Col];
}
//---------------------------------------------------------------------

template<class TYPE> void CArray2<TYPE>::Set(DWORD Col, DWORD Row, const TYPE& Elm)
{
	n_assert(pData && Col < Width && Row < Height);
	pData[Row * Width + Col] = Elm;
}
//---------------------------------------------------------------------

#endif
