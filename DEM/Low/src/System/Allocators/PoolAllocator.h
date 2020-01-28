#pragma once
#include <StdDEM.h>

// Simple pool for fast allocation of multiple frequently used objects, typically small.

//???template bytesize instead of T? can wrap bytesize allocator into a T wrapper

template <class T, UPTR ChunkSize = 128>
class CPoolAllocator
{
	static_assert(ChunkSize > 0);

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

	CChunkNode<T>*	Chunks = nullptr;
	CRecord<T>*		FreeRecords = nullptr;

#ifdef _DEBUG
	UPTR			CurrAllocatedCount = 0;
#endif

public:

	CPoolAllocator();
	~CPoolAllocator() { Clear(); }

	void*	Allocate();
	void	Free(void* pPtr);
	T*		Construct();
	T*		Construct(const T& Other);
	T*		Construct(T&& Other);
	void	Destroy(T* pPtr);
	void	Clear();
};

template<class T, UPTR ChunkSize>
CPoolAllocator<T, ChunkSize>::CPoolAllocator():
#ifdef _DEBUG
	CurrAllocatedCount(0),
#endif
	Chunks(nullptr),
	FreeRecords(nullptr)
{
	n_assert(ChunkSize > 0);
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize>
void* CPoolAllocator<T, ChunkSize>::Allocate()
{
	T* AllocatedRec(nullptr);

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
			for (; Curr < End; ++Curr)
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

template<class T, UPTR ChunkSize>
inline void CPoolAllocator<T, ChunkSize>::Free(void* pPtr)
{
	if (!pPtr) return;

#ifdef _DEBUG
	--CurrAllocatedCount;
#endif

	((CRecord<T>*)pPtr)->Next = FreeRecords;
	FreeRecords = (CRecord<T>*)pPtr;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize>
inline T* CPoolAllocator<T, ChunkSize>::Construct()
{
	T* pNew = (T*)Allocate();
	n_placement_new(pNew, T);
	return pNew;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize>
inline T* CPoolAllocator<T, ChunkSize>::Construct(const T& Other)
{
	T* pNew = (T*)Allocate();
	n_placement_new(pNew, T)(Other);
	return pNew;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize>
inline T* CPoolAllocator<T, ChunkSize>::Construct(T&& Other)
{
	T* pNew = (T*)Allocate();
	n_placement_new(pNew, T)(std::move(Other));
	return pNew;
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize>
inline void CPoolAllocator<T, ChunkSize>::Destroy(T* pPtr)
{
	if (pPtr)
	{
		pPtr->~T();
		Free(pPtr);
	}
}
//---------------------------------------------------------------------

template<class T, UPTR ChunkSize>
inline void CPoolAllocator<T, ChunkSize>::Clear()
{
#ifdef _DEBUG
	if (CurrAllocatedCount > 0)
		Sys::Error("Pool reports %d unreleased records", CurrAllocatedCount);
#endif

	if (Chunks)
	{
		n_delete(Chunks);
		Chunks = nullptr;
	}
}
//---------------------------------------------------------------------
