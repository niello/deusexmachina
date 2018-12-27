#pragma once
#ifndef __DEM_L1_TEMPLATE_TABLE_H__
#define __DEM_L1_TEMPLATE_TABLE_H__

#include <Data/StringID.h>
#include <Data/Array2.h>
#include <Data/Dictionary.h>

// Template table with fixed size, 2D array with name and optionally named columns.

namespace Data
{

template<class T>
class CTableT: public CArray2<T>
{
public:

	// Made public for loader access
	CString				Name;
	CDict<CStrID, int>	ColMap;
	CDict<CStrID, int>	RowMap;

	CTableT() {}
	CTableT(UPTR Cols, UPTR Rows): CArray2(Cols, Rows) {}

	UPTR	GetColumnCount() const { return Width; }
	UPTR	GetRowCount() const { return Height; }

	T&		Cell(CStrID ColName, CStrID RowName) const { return At(ColMap.FindIndex(ColName), RowMap.FindIndex(RowName)); }
	T&		Cell(CStrID ColName, UPTR RowIdx) const { return At(ColMap.FindIndex(ColName), RowIdx); }
	T&		Cell(UPTR ColIdx, CStrID RowName) const { return At(ColIdx, RowMap.FindIndex(RowName)); }
	T&		Cell(UPTR ColIdx, UPTR RowIdx) const { return At(ColIdx, RowIdx); }

	template<class TOut>
	bool	Convert(Data::CTableT<TOut>& Out, bool(*Convertor)(const T&, TOut&));

	T&		GetElm(UPTR Idx) const { return pData[Idx]; } // Mainly for internal use
};

template<class T> template<class TOut>
bool CTableT<T>::Convert(Data::CTableT<TOut>& Out, bool(*Convertor)(const T&, TOut&))
{
	Out.SetSize(Width, Height);
	for (UPTR i = 0; i < Width * Height; ++i)
		if (!Convertor(pData[i], Out.GetElm(i))) FAIL;
	OK;
}
//---------------------------------------------------------------------

}

#endif
