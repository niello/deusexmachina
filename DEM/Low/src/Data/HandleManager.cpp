#include "HandleManager.h"

namespace Data
{

HHandle CHandleManager::OpenHandle(void* pData)
{
	CHandleRec* pRec;
	UPTR Index, Magic;
	if (FirstFreeIndex >= INVALID_HALF_INDEX)
	{
		// Maximum handle count is limited by index bits, which are a half of total handle bits.
		// The last index is reserved for an invalid one.
		if (HandleRecs.GetCount() >= INVALID_HALF_INDEX) return 0;

		Index = HandleRecs.GetCount();
		Magic = 1;
		pRec = HandleRecs.Reserve(1);
	}
	else
	{
		pRec = &HandleRecs[FirstFreeIndex];
		Index = FirstFreeIndex;
		UPTR MNFI = pRec->MagicAndNextFreeIndex;
		FirstFreeIndex = UPTR_LOW_HALF(MNFI);
		if (FirstFreeIndex >= INVALID_HALF_INDEX) LastFreeIndex = INVALID_HALF_INDEX;
		Magic = UPTR_HIGH_HALF(MNFI);
	}

	Magic <<= HalfRegisterBits;

	pRec->pData = pData;
	pRec->MagicAndNextFreeIndex = Magic | INVALID_HALF_INDEX;

	return Magic | Index;
}
//---------------------------------------------------------------------

HHandle CHandleManager::FindHandle(void* pData) const
{
	for (UPTR i = 0; i < HandleRecs.GetCount(); ++i)
		if (HandleRecs[i].pData == pData)
		{
			NOT_IMPLEMENTED;
		}

	return INVALID_HANDLE;
}
//---------------------------------------------------------------------

void CHandleManager::CloseHandle(HHandle Handle)
{
	UPTR Index = UPTR_LOW_HALF(Handle);
	if (Index >= (UPTR)HandleRecs.GetCount()) return;
	UPTR Magic = UPTR_HIGH_HALF(Handle);
	CHandleRec& Rec = HandleRecs[Index];
	if (Magic != UPTR_HIGH_HALF(Rec.MagicAndNextFreeIndex)) return;

	++Magic;
	if (!Magic) Magic = 1; 

	Rec.pData = nullptr;
	Rec.MagicAndNextFreeIndex = (Magic << HalfRegisterBits) | INVALID_HALF_INDEX;

	if (LastFreeIndex < INVALID_HALF_INDEX)
	{
		CHandleRec& LastFreeRec = HandleRecs[LastFreeIndex];
		UPTR LFRMagic = UPTR_HIGH_HALF(LastFreeRec.MagicAndNextFreeIndex);
		LastFreeRec.MagicAndNextFreeIndex = (LFRMagic << HalfRegisterBits) | Index;
	}

	LastFreeIndex = Index;
	if (FirstFreeIndex >= INVALID_HALF_INDEX) FirstFreeIndex = Index;
}
//---------------------------------------------------------------------

}