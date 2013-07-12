#pragma once
#ifndef __DEM_L1_TEMPLATE_TABLE_H__
#define __DEM_L1_TEMPLATE_TABLE_H__

#include <Data/SimpleString.h>
#include <Data/StringID.h>
#include <util/narray2.h>
#include <Data/Dictionary.h>

// Template table with fixed size, 2D array with name and optionally named columns.

namespace Data
{

template<class T>
class CTableT: public nArray2<T>
{
public:

	// Made public for loader access
	CSimpleString		Name;
	CDict<CStrID, int>	ColMap;
	CDict<CStrID, int>	RowMap;

	CTableT() {}
	CTableT(DWORD Cols, DWORD Rows): nArray2(Cols, Rows) {}

	DWORD	GetColumnCount() const { return Width; }
	DWORD	GetRowCount() const { return Height; }

	T&		Cell(CStrID ColName, CStrID RowName) const { return At(ColMap.FindIndex(ColName), RowMap.FindIndex(RowName)); }
	T&		Cell(CStrID ColName, DWORD RowIdx) const { return At(ColMap.FindIndex(ColName), RowIdx); }
	T&		Cell(DWORD ColIdx, CStrID RowName) const { return At(ColIdx, RowMap.FindIndex(RowName)); }
	T&		Cell(DWORD ColIdx, DWORD RowIdx) const { return At(ColIdx, RowIdx); }

	template<class TOut>
	bool	Convert(Data::CTableT<TOut>& Out, bool(*Convertor)(const T&, TOut&));

	T&		GetElm(uint Idx) const { return pData[Idx]; } // Mainly for internal use
};

template<class T> template<class TOut>
bool CTableT<T>::Convert(Data::CTableT<TOut>& Out, bool(*Convertor)(const T&, TOut&))
{
	Out.SetSize(Width, Height);
	for (uint i = 0; i < Width * Height; ++i)
		if (!Convertor(pData[i], Out.GetElm(i))) FAIL;
	OK;
}
//---------------------------------------------------------------------

}

#endif
