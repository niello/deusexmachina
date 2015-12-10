#pragma once
#ifndef __DEM_L1_ALLOC_POOL_H__
#define __DEM_L1_ALLOC_POOL_H__

#include <StdDEM.h>

// Simple pool for fast allocation of multiple frequently used objects, typically small.

//!!!TODO: use MaxChunks!
//???template bytesize instead of T? can wrap bytesize allocator into a T wrapper

template <class T, UPTR ChunkSize = 128, UPTR MaxChunks = 64>
class CPoolAllocator
{
protected:

	template <class T>
	struct CRecord
	{
		union
		{
			char		Object[sizeof(T)]; // T Object; causes constructor error
			CRecord<T>*	Next;
		};

		operator T*() { return (T*)Object; } 
	};

	template <class T>
	struct CChunkNode
	{
		CChunkNode<T>*	Next;
		CRecord<T>*		ChunkRecords;
		~CChunkNode() { if (Next) n_delete(Next); n_delete_array(ChunkRecords); }
	};

	CChunkNode<T>*	Chunks;
	CRecord<T>*		FreeRecords;

#ifdef _DEBUG
	UPTR			CurrAllocatedCount;
#endif

public:

	CPoolAllocator();
	~CPoolAllocator() { Clear(); }

	void*	Allocate();
	void	Free(void* pPtr);
	T*		Construct();
	T*		Construct(const T& Other);
	void	Destroy(T* pPtr);
	void	Clear();
};

template<class T, UPTR ChunkSize, UPTR MaxChunks>
CPoolAllocator<T, ChunkSize, MaxChunks>::CPoolAllocator():
#ifdef _DEBUG
	CurrAllocatedCount(0),
#endif
	Chunks(NULL),
	FreeRecords(NULL)
{
	n_assert(ChunkSize > 0);
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize, UPTR MaxChunks>
void* CPoolAllocator<T, ChunkSize, MaxChunks>::Allocate()
{
	T* AllocatedRec(NULL);

	if (FreeRecords)
	{
		AllocatedRec = *FreeRecords;
		FreeRecords = FreeRecords->Next;
	}
	else
	{
		CChunkNode<T>* NewChunk = n_new(CChunkNode<T>());
		NewChunk->ChunkRecords = n_new_array(CRecord<T>, ChunkSize);
		NewChunk->Next = Chunks;
		Chunks = NewChunk;

		CRecord<T>* Curr = NewChunk->ChunkRecords;
		CRecord<T>*	End = NewChunk->ChunkRecords + ChunkSize - 2;
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
	++CurrAllocatedCount;
#endif

	return AllocatedRec;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize, UPTR MaxChunks>
inline void CPoolAllocator<T, ChunkSize, MaxChunks>::Free(void* pPtr)
{
	if (!pPtr) return;

#ifdef _DEBUG
	--CurrAllocatedCount;
#endif

	((CRecord<T>*)pPtr)->Next = FreeRecords;
	FreeRecords = (CRecord<T>*)pPtr;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize, UPTR MaxChunks>
inline T* CPoolAllocator<T, ChunkSize, MaxChunks>::Construct()
{
	T* pNew = (T*)Allocate();
	n_placement_new(pNew, T);
	return pNew;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize, UPTR MaxChunks>
inline T* CPoolAllocator<T, ChunkSize, MaxChunks>::Construct(const T& Other)
{
	T* pNew = (T*)Allocate();
	n_placement_new(pNew, T)(Other);
	return pNew;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize, UPTR MaxChunks>
inline void CPoolAllocator<T, ChunkSize, MaxChunks>::Destroy(T* pPtr)
{
	if (pPtr)
	{
		pPtr->~T();
		Free(pPtr);
	}
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize, UPTR MaxChunks>
inline void CPoolAllocator<T, ChunkSize, MaxChunks>::Clear()
{
#ifdef _DEBUG
	if (CurrAllocatedCount > 0)
		Sys::Error("Pool reports %d unreleased records", CurrAllocatedCount);
#endif

	if (Chunks) n_delete(Chunks);
}
//---------------------------------------------------------------------

#endif
