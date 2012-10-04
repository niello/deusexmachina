#pragma once
#ifndef __DEM_L1_STRID_STORAGE_H__
#define __DEM_L1_STRID_STORAGE_H__

#include "StringID.h"

#include <util/nhashmap2.h>

#define STR_BLOCK_COUNT	64
#define STR_BLOCK_SIZE	8192

namespace Data //???need?
{

class CStrIDStorage
{
protected:

	//???CPool<LPCSTR>					Table;
	nHashMap2<CStringID>	Map;
	LPSTR					Block[STR_BLOCK_COUNT];
	int						BlockIndex,
							BlockPosition;

	LPCSTR		StoreString(LPCSTR String);

public:

	CStrIDStorage();
	~CStrIDStorage();

	CStringID	GetIDByString(LPCSTR String);
	CStringID	AddString(LPCSTR String);
};

}

#endif