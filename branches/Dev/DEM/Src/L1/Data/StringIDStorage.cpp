#include "StringIDStorage.h"

#include <memory.h>

namespace Data
{

CStringIDStorage::CStringIDStorage():
	Chains(512),
#ifdef _DEBUG
	Stats_RecordCount(0),
	Stats_CollisionCount(0),
#endif
	BlockIndex(0),
	BlockPosition(0)
{
	memset(Block, 0, STR_BLOCK_COUNT * sizeof(const char*));
}
//---------------------------------------------------------------------

const char* CStringIDStorage::StoreString(const char* pString)
{
	int Len = strlen(pString) + 1;

	if (Len > STR_BLOCK_SIZE) return NULL;

	if (BlockPosition + Len > STR_BLOCK_SIZE)
	{
		if (BlockIndex + 1 >= STR_BLOCK_COUNT) return NULL;
		++BlockIndex;
		BlockPosition = 0;
	}

	if (Block[BlockIndex] == NULL)
	{
		Block[BlockIndex] = (char*)n_malloc(STR_BLOCK_SIZE); //!!!heap mgr, dlmalloc!
		BlockPosition = 0;
	}

	char* pStoredStr = Block[BlockIndex] + BlockPosition;
	memcpy(pStoredStr, pString, Len);
	BlockPosition += Len;

	return pStoredStr;
}
//---------------------------------------------------------------------

CStringID CStringIDStorage::Get(const char* pString) const
{
	unsigned int HashValue = Hash(pString);
	CArray<CRecord>& Chain = Chains[HashValue % Chains.GetCount()];
	if (Chain.GetCount() == 1)
	{
		CRecord& Rec = Chain[0];
		if (Rec.Hash == HashValue && !strcmp(Rec.pStr, pString)) return CStringID(Rec.pStr, 0, 0);
	}
	else if (Chain.GetCount() > 1)
	{
		CRecord CmpRec;
		CmpRec.Hash = HashValue;
		CmpRec.pStr = pString;
		int Idx = Chain.FindIndexSorted(CmpRec);
		if (Idx != INVALID_INDEX) return CStringID(Chain[Idx].pStr, 0, 0);
	}
	return CStringID::Empty;
}
//---------------------------------------------------------------------

CStringID CStringIDStorage::GetOrAdd(const char* pString)
{
	unsigned int HashValue = Hash(pString);
	CArray<CRecord>& Chain = Chains[HashValue % Chains.GetCount()];
	int Idx = 0;
	if (Chain.GetCount() == 1)
	{
		CRecord& Rec = Chain[0];
		if (Rec.Hash == HashValue && !strcmp(Rec.pStr, pString)) return CStringID(Rec.pStr, 0, 0);
		if (strcmp(Rec.pStr, pString) < 0) Idx = 1;
		DBG_ONLY(++Stats_CollisionCount);
	}
	else if (Chain.GetCount() > 1)
	{
		CRecord CmpRec;
		CmpRec.Hash = HashValue;
		CmpRec.pStr = pString;
		bool HasEqual;
		Idx = Chain.FindClosestIndexSorted(CmpRec, &HasEqual);
		if (HasEqual) return CStringID(Chain[Idx].pStr, 0, 0);
		DBG_ONLY(++Stats_CollisionCount);
	}

	const char* pStoredStr = StoreString(pString);

	CRecord CmpRec;
	CmpRec.Hash = HashValue;
	CmpRec.pStr = pStoredStr;
	CArray<CRecord>::CIterator It = Chain.Insert(Idx, CmpRec);
	DBG_ONLY(++Stats_RecordCount);

	return CStringID(pStoredStr, 0, 0);
}
//---------------------------------------------------------------------

}