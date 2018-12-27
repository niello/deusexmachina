#pragma once
#ifndef __DEM_L1_DATA_HANDLE_MANAGER_H__
#define __DEM_L1_DATA_HANDLE_MANAGER_H__

#include <Data/Array.h>

// Handles are safer than pointers, they virtually avoid dangling problem,
// can uniquely identify an object in a manager scope, and can be dereferenced
// to an object pointer with a fast conversion.
// Zero HHandle is a NULL, or empty one. It is also defined as an INVALID_HANDLE.

namespace Data
{

class CHandleManager
{
protected:

	struct CHandleRec
	{
		void*	pData;
		UPTR	MagicAndNextFreeIndex;
	};

	CArray<CHandleRec>	HandleRecs;
	UPTR				FirstFreeIndex;
	UPTR				LastFreeIndex;

public:

	CHandleManager(UPTR GrowSize = 128): HandleRecs(0, GrowSize), FirstFreeIndex(INVALID_HALF_INDEX), LastFreeIndex(INVALID_HALF_INDEX) {}

	HHandle		OpenHandle(void* pData);
	HHandle		FindHandle(void* pData) const;
	void*		GetHandleData(HHandle Handle) const;
	bool		IsHandleValid(HHandle Handle) const;
	void		CloseHandle(HHandle Handle);
	void		Clear();
};

inline void* CHandleManager::GetHandleData(HHandle Handle) const
{
	UPTR Index = UPTR_LOW_HALF(Handle);
	if (Index >= HandleRecs.GetCount()) return NULL;
	CHandleRec& Rec = HandleRecs[Index];
	return UPTR_HIGH_HALF(Handle) == UPTR_HIGH_HALF(Rec.MagicAndNextFreeIndex) ? Rec.pData : NULL;
}
//---------------------------------------------------------------------

inline bool CHandleManager::IsHandleValid(HHandle Handle) const
{
	UPTR Index = UPTR_LOW_HALF(Handle);
	return
		Index < HandleRecs.GetCount() &&
		UPTR_HIGH_HALF(Handle) == UPTR_HIGH_HALF(HandleRecs[Index].MagicAndNextFreeIndex);
}
//---------------------------------------------------------------------

inline void CHandleManager::Clear()
{
	HandleRecs.Clear();
	FirstFreeIndex = INVALID_HALF_INDEX;
	LastFreeIndex = INVALID_HALF_INDEX;
}
//---------------------------------------------------------------------

}

#endif
