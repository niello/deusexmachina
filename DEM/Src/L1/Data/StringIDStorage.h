#pragma once
#ifndef __DEM_L1_STRID_STORAGE_H__
#define __DEM_L1_STRID_STORAGE_H__

#include "StringID.h"
#include <Data/FixedArray.h>
#include <Data/Array.h>

#define STR_BLOCK_COUNT	64
#define STR_BLOCK_SIZE	8192

namespace Data
{

class CStringIDStorage
{
protected:

	struct CRecord
	{
		UPTR		Hash;
		const char*	pStr;	// For already stored StrIDs it is a StrID-valid pointer, for new records it is a regular char*

		bool operator ==(const CRecord& Other) const { return Hash == Other.Hash && strcmp(pStr, Other.pStr) == 0; }
		bool operator <(const CRecord& Other) const { return strcmp(pStr, Other.pStr) < 0; }
		bool operator >(const CRecord& Other) const { return strcmp(pStr, Other.pStr) > 0; }
	};

	CFixedArray<CArray<CRecord>>	Chains; //???linked lists?

	char*							Block[STR_BLOCK_COUNT];
	int								BlockIndex;
	int								BlockPosition;

#ifdef _DEBUG
	UPTR							Stats_RecordCount;
	UPTR							Stats_CollisionCount;
#endif

	const char*	StoreString(const char* pString);

public:

	CStringIDStorage();
	~CStringIDStorage() { for (int i = 0; i <= BlockIndex; ++i) n_free(Block[i]); }

	CStringID	Get(const char* pString) const;
	CStringID	GetOrAdd(const char* pString);
};

}

#endif