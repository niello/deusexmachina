#pragma once
#ifndef __DEM_L1_STRID_STORAGE_H__
#define __DEM_L1_STRID_STORAGE_H__

#include "StringID.h"

#include <Data/SimpleString.h>
#include <Data/HashTable.h>

#define STR_BLOCK_COUNT	64
#define STR_BLOCK_SIZE	8192

namespace Data
{

class CStringIDStorage
{
protected:

	CHashTable<CSimpleString, CStringID>	Map;
	LPSTR									Block[STR_BLOCK_COUNT];
	int										BlockIndex;
	int										BlockPosition;

	LPCSTR		StoreString(LPCSTR String);

public:

	CStringIDStorage();
	~CStringIDStorage();

	bool		GetIDByString(LPCSTR String, CStringID& OutID) const { return Map.Get(String, OutID); }
	CStringID	GetIDByString(LPCSTR String) const { CStringID ID; Map.Get(String, ID); return ID; }
	CStringID	AddString(LPCSTR String);
};

inline CStringID CStringIDStorage::AddString(LPCSTR String)
{
	CStringID& StrID = Map.At(String);
	if (!StrID.IsValid()) StrID = CStringID(StoreString(String), 0, 0);
	return StrID;
}
//---------------------------------------------------------------------

}

#endif