#pragma once
#include "StringID.h"
#include <vector>

#define STR_BLOCK_COUNT	64
#define STR_BLOCK_SIZE	8192

namespace Data
{

class CStringIDStorage final
{
protected:

	struct CRecord
	{
		uintptr_t	Hash;
		const char*	pStr;	// For already stored StrIDs it is a StrID-valid pointer, for new records it is a regular char*

		bool operator ==(const CRecord& Other) const { return Hash == Other.Hash && strcmp(pStr, Other.pStr) == 0; }
		bool operator <(const CRecord& Other) const { return strcmp(pStr, Other.pStr) < 0; }
		bool operator >(const CRecord& Other) const { return strcmp(pStr, Other.pStr) > 0; }
	};

	std::vector<std::vector<CRecord>>	Map; //???linked list chains?

	char*							Block[STR_BLOCK_COUNT];
	int								BlockIndex;
	int								BlockPosition;

#ifdef _DEBUG
	uintptr_t						Stats_RecordCount;
	uintptr_t						Stats_CollisionCount;
#endif

	const char*	StoreString(const char* pString);

public:

	CStringIDStorage();
	~CStringIDStorage() { for (int i = 0; i <= BlockIndex; ++i) free(Block[i]); }

	CStringID	Get(const char* pString) const;
	CStringID	GetOrAdd(const char* pString);
};

}
