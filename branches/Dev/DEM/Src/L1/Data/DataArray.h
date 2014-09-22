#pragma once
#ifndef __DEM_L1_DATA_ARRAY_H__
#define __DEM_L1_DATA_ARRAY_H__

#include <Data/RefCounted.h>
#include <Data/Data.h>

// Array of variant variables

namespace Data
{

class CDataArray: public CArray<CData>, public CRefCounted
{
public:

	CDataArray() {}
	CDataArray(DWORD _Count, DWORD _GrowSize): CArray(_Count, _GrowSize) {}
	CDataArray(const CDataArray& Other) { Copy(Other); }

	template<class T>
	void			FillArray(CArray<T>& OutArray) const;

	CData&			Get(int Index) { return At(Index); }
	const CData&	Get(int Index) const { return At(Index); }
	template<class T>
	T&				Get(int Index) { return At(Index).GetValue<T>(); }
	template<class T>
	const T&		Get(int Index) const { return At(Index).GetValue<T>(); }
};
//---------------------------------------------------------------------

typedef Ptr<CDataArray> PDataArray;

template<class T> inline void CDataArray::FillArray(CArray<T>& OutArray) const
{
	for (CIterator It = Begin(); It != End(); It++)
		OutArray.Add((*It).GetValue<T>());
}
//---------------------------------------------------------------------

}

DECLARE_TYPE(PDataArray, 10)
#define TArray	DATA_TYPE(PDataArray)

#endif
