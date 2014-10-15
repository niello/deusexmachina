#pragma once
#ifndef __DEM_L1_POOL_H__
#define __DEM_L1_POOL_H__

#include <StdDEM.h>

// Simple pool for fast allocation of multiple frequently used objects, typically small.

//!!!TODO: use MaxChunks!

template <unsigned short InstanceSize, DWORD ChunkSize = 128, DWORD MaxChunks = 64>
class CPoolAllocator
{
protected:

	template <unsigned short InstanceSize>
	struct CRecord
	{
		union
		{
			char					Object[InstanceSize];
			CRecord<InstanceSize>*	Next;
		};
	};

	template <unsigned short InstanceSize>
	struct CChunkNode
	{
		CChunkNode<InstanceSize>*	Next;
		CRecord<InstanceSize>*		ChunkRecords;
		~CChunkNode() { if (Next) n_delete(Next); n_delete_array(ChunkRecords); }
	};

	CChunkNode<InstanceSize>*	Chunks;
	CRecord<InstanceSize>*		FreeRecords;

#ifdef _DEBUG
	DWORD			AllocationsCount;
	DWORD			ReleasesCount;
#endif

	void*	AllocRecord();

public:

	CPoolAllocator();
	~CPoolAllocator() { Clear(); }

	void	Destroy(void* Handle);
	void	Clear();

	//!!!use static_assert when possible!
	template<class T>
	T*		Construct()
	{
		n_assert_dbg(sizeof(T) <= InstanceSize);
		void* Rec = AllocRecord();
		n_placement_new(Rec, T);
		return (T*)Rec;
	}

	//!!!use static_assert when possible!
	template<class T>
	T*		Construct(const T& Other)
	{
		n_assert_dbg(sizeof(T) <= InstanceSize);
		void* Rec = AllocRecord();
		n_placement_new(Rec, T)(Other);
		return (T*)Rec;
	}
};

template<unsigned short InstanceSize, DWORD ChunkSize, DWORD MaxChunks>
CPoolAllocator<InstanceSize, ChunkSize, MaxChunks>::CPoolAllocator():
#ifdef _DEBUG
	AllocationsCount(0),
	ReleasesCount(0),
#endif
	Chunks(NULL),
	FreeRecords(NULL)
{
	n_assert(InstanceSize > 0 && ChunkSize > 0);
}
//---------------------------------------------------------------------

template<unsigned short InstanceSize, DWORD ChunkSize, DWORD MaxChunks>
void* CPoolAllocator<InstanceSize, ChunkSize, MaxChunks>::AllocRecord()
{
	void* AllocatedRec(NULL);

	if (FreeRecords)
	{
		AllocatedRec = *FreeRecords;
		FreeRecords = FreeRecords->Next;
	}
	else
	{
		//???need all these allocations?
		CChunkNode<InstanceSize>* NewChunk = n_new(CChunkNode<InstanceSize>());
		NewChunk->ChunkRecords = n_new_array(CRecord<InstanceSize>, ChunkSize);
		NewChunk->Next = Chunks;
		Chunks = NewChunk;

		CRecord<InstanceSize>* Curr = NewChunk->ChunkRecords;
		CRecord<InstanceSize>*End = NewChunk->ChunkRecords + ChunkSize - 2;
		if (ChunkSize > 1)
		{
			Curr->Next = FreeRecords;
			for (; Curr < End; Curr++)
				(Curr + 1)->Next = Curr;
			FreeRecords = End;
		}
		AllocatedRec = *(End + 1);
	}

#ifdef _DEBUG
	AllocationsCount++;
#endif

	//???need?
	n_assert(AllocatedRec);

	return AllocatedRec;
}
//---------------------------------------------------------------------

template<unsigned short InstanceSize, DWORD ChunkSize, DWORD MaxChunks>
inline void CPoolAllocator<T, ChunkSize, MaxChunks>::Destroy(T* Handle)
{
	if (!Handle) return;

#ifdef _DEBUG
	ReleasesCount++;
#endif

	//!!!assert is valid pool element - for dbg!
	Handle->~T();
	((CRecord<InstanceSize>*)Handle)->Next = FreeRecords;
	FreeRecords = (CRecord<InstanceSize>*)Handle;
}
//---------------------------------------------------------------------

template<unsigned short InstanceSize, DWORD ChunkSize, DWORD MaxChunks>
inline void CPoolAllocator<T, ChunkSize, MaxChunks>::Clear()
{
#ifdef _DEBUG
	if (AllocationsCount != ReleasesCount)
		Sys::Error("Pool reports %d allocations and %d releases", AllocationsCount, ReleasesCount);
#endif

	if (Chunks) n_delete(Chunks);
}
//---------------------------------------------------------------------

#endif
