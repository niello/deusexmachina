#include "StringIDStorage.h"

#include <memory.h>

namespace Data
{

CStringIDStorage::CStringIDStorage(): Map(CStrID(""), 512)
{
	memset(Block, 0, STR_BLOCK_COUNT * sizeof(LPCSTR));
	BlockIndex = 0;
	BlockPosition = 0;
}
//---------------------------------------------------------------------

CStringIDStorage::~CStringIDStorage()
{
	for (int i = 0; i <= BlockIndex; ++i) n_free(Block[i]);
}
//---------------------------------------------------------------------

CStringID CStringIDStorage::AddString(LPCSTR String)
{
	//!!!can calc hash once!

	CStringID ExistingID;
	if (Map.Get(String, ExistingID)) return ExistingID;

	CStringID New(StoreString(String), 0, 0);
	Map[New.CStr()] = New;

	return New;
}
//---------------------------------------------------------------------

LPCSTR CStringIDStorage::StoreString(LPCSTR String)
{
	int Len = strlen(String) + 1;

	if (Len > STR_BLOCK_SIZE) return NULL;

	if (BlockPosition + Len > STR_BLOCK_SIZE)
	{
		if (BlockIndex + 1 >= STR_BLOCK_COUNT) return NULL;
		++BlockIndex;
		BlockPosition = 0;
	}

	if (Block[BlockIndex] == NULL)
	{
		Block[BlockIndex] = (char*)n_malloc(STR_BLOCK_SIZE); //!!!heap mgr, Doug Lea!
		BlockPosition = 0;
	}

	LPSTR Str = Block[BlockIndex] + BlockPosition;
	memcpy(Str, String, Len);
	BlockPosition += Len;

	return Str;
}
//---------------------------------------------------------------------

}