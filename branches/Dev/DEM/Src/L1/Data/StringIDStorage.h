#pragma once
#ifndef __DEM_L1_STRID_STORAGE_H__
#define __DEM_L1_STRID_STORAGE_H__

#include "StringID.h"

#include <util/HashMap.h>

#define STR_BLOCK_COUNT	64
#define STR_BLOCK_SIZE	8192

namespace Data //???need?
{

class CStringIDStorage
{
protected:

	//???CPool<LPCSTR>	Table;
	CHashMap<CStringID>	Map;
	LPSTR				Block[STR_BLOCK_COUNT];
	int					BlockIndex;
	int					BlockPosition;

	LPCSTR		StoreString(LPCSTR String);

public:

	CStringIDStorage();
	~CStringIDStorage();

	bool		GetIDByString(LPCSTR String, CStringID& OutID) { return Map.Get(String, OutID); }
	CStringID	GetIDByString(LPCSTR String) { CStringID ID; Map.Get(String, ID); return ID; }
	CStringID	AddString(LPCSTR String);
};

}

#endif