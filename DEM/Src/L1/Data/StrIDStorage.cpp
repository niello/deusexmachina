#include "StrIDStorage.h"
#include <memory.h>

namespace Data //???need?
{

CStrIDStorage::CStrIDStorage(): Map(CStrID(""), 512)
{
	memset(Block, 0, STR_BLOCK_COUNT * sizeof(LPCSTR));
	BlockIndex=
	BlockPosition=0;
}
//---------------------------------------------------------------------

CStrIDStorage::~CStrIDStorage()
{
	for (int i = 0; i <= BlockIndex; i++) n_free(Block[i]);
}
//---------------------------------------------------------------------

CStringID CStrIDStorage::GetIDByString(LPCSTR String)
{
	if (Map.Contains(String)) return Map[String];
	return CStringID();
}
//---------------------------------------------------------------------

CStringID CStrIDStorage::AddString(LPCSTR String)
{
	if (Map.Contains(String)) return Map[String];

	CStringID New(StoreString(String), 0, 0);
	Map[New.CStr()] = New;

	return New;
}
//---------------------------------------------------------------------

LPCSTR CStrIDStorage::StoreString(LPCSTR String)
{
	int len = strlen(String) + 1;

	if (len > STR_BLOCK_SIZE) return NULL;

	if (BlockPosition + len > STR_BLOCK_SIZE)
	{
		if (BlockIndex + 1 >= STR_BLOCK_COUNT) return NULL;
		BlockIndex++;
		BlockPosition = 0;
	}

	if (Block[BlockIndex] == NULL)
	{
		Block[BlockIndex] = (char*)n_malloc(STR_BLOCK_SIZE);//!!!heap mgr, Doug Lea!
		BlockPosition = 0;
	}

	LPSTR Str = Block[BlockIndex]+BlockPosition;
	memcpy(Str, String, len);
	BlockPosition += len;

	return Str;
}
//---------------------------------------------------------------------

}