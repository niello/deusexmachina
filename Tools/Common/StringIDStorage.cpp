#include "StringIDStorage.h"
#include <MurmurHash3.h>
#include <memory.h>
#include <algorithm>

static inline unsigned int Hash(const void* pData, int Length)
{
	unsigned int Result;
	MurmurHash3_x86_32(pData, Length, 0xB0F57EE3, &Result);
	return Result;
}
//---------------------------------------------------------------------

namespace Data
{

CStringIDStorage::CStringIDStorage():
	Map(512),
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

	if (Len > STR_BLOCK_SIZE) return nullptr;

	if (BlockPosition + Len > STR_BLOCK_SIZE)
	{
		if (BlockIndex + 1 >= STR_BLOCK_COUNT) return nullptr;
		++BlockIndex;
		BlockPosition = 0;
	}

	if (Block[BlockIndex] == nullptr)
	{
		Block[BlockIndex] = static_cast<char*>(malloc(STR_BLOCK_SIZE));
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
	unsigned int HashValue = Hash(pString, strlen(pString));
	auto& Chain = Map[HashValue % Map.size()];
	if (Chain.size() == 1)
	{
		const CRecord& Rec = Chain[0];
		if (Rec.Hash == HashValue && !strcmp(Rec.pStr, pString)) return CStringID(Rec.pStr, 0, 0);
	}
	else if (Chain.size() > 1)
	{
		CRecord CmpRec;
		CmpRec.Hash = HashValue;
		CmpRec.pStr = pString;
		auto It = std::lower_bound(Chain.cbegin(), Chain.cend(), CmpRec);
		if (It != Chain.cend()) return CStringID(It->pStr, 0, 0);
	}
	return CStringID::Empty;
}
//---------------------------------------------------------------------

CStringID CStringIDStorage::GetOrAdd(const char* pString)
{
	unsigned int HashValue = Hash(pString, strlen(pString));
	auto& Chain = Map[HashValue % Map.size()];
	auto InsertPos = Chain.begin();
	if (Chain.size() == 1)
	{
		CRecord& Rec = Chain[0];
		if (Rec.Hash == HashValue && !strcmp(Rec.pStr, pString)) return CStringID(Rec.pStr, 0, 0);
		if (strcmp(Rec.pStr, pString) < 0) ++InsertPos;
#ifdef _DEBUG
		++Stats_CollisionCount;
#endif
	}
	else if (Chain.size() > 1)
	{
		// If equal element found, FindClosestIndexSorted() returns the next index for
		// insertion optimality. So, equal element must be referenced as [Idx - 1].
		CRecord CmpRec;
		CmpRec.Hash = HashValue;
		CmpRec.pStr = pString;

		InsertPos = std::lower_bound(Chain.begin(), Chain.end(), CmpRec);
		if (InsertPos != Chain.end() && *InsertPos == CmpRec)
			return CStringID(InsertPos->pStr, 0, 0);
#ifdef _DEBUG
		++Stats_CollisionCount;
#endif
	}

	const char* pStoredStr = StoreString(pString);

	CRecord CmpRec;
	CmpRec.Hash = HashValue;
	CmpRec.pStr = pStoredStr;
	Chain.insert(InsertPos, std::move(CmpRec));
#ifdef _DEBUG
	++Stats_RecordCount;
#endif

	return CStringID(pStoredStr, 0, 0);
}
//---------------------------------------------------------------------

}