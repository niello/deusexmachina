#pragma once
#ifndef __DEM_L1_DATA_ARRAY_H__
#define __DEM_L1_DATA_ARRAY_H__

#include <Core/RefCounted.h>
#include "Data.h"

// Array of variant variables

namespace Data
{

class CDataArray: public nArray<CData>, public Core::CRefCounted
{
public:

	template<class T>
	void			FillArray(nArray<T>& OutArray) const;

	CData&			Get(int Index) { return At(Index); }
	const CData&	Get(int Index) const { return this->operator [](Index); }

	//template<class T> const T& operator [](int Index) const {}
	//const CData& operator [](int Index) const { return (const CData&)At(Index); }
};
//---------------------------------------------------------------------

typedef Ptr<CDataArray> PDataArray;

template<class T> inline void CDataArray::FillArray(nArray<T>& OutArray) const
{
	for (iterator It = Begin(); It != End(); It++)
		OutArray.Append((*It).GetValue<T>());
}
//---------------------------------------------------------------------

}

DECLARE_TYPE(PDataArray, 10)
#define TArray	DATA_TYPE(PDataArray)

#endif
