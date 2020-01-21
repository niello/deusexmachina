#pragma once
#include <StdDEM.h>

// Handle array combines a growable array and a pool allocator. For element referencing
// it provides persistent handles which are never invalidated, unlike pointers or iterators.
// Handle dereferencing is very fast. Don't store raw pointers obtained through handles.
// There are template parameters:
// T               - value type
// H               - handle internal representation, must be any unsigned integral type
// IndexBits       - amount of bits in H reserved for element indexing. Determines a maximum
//                   possible element count. Other bits in a handle are for slot reuse, so
//                   try to set IndexBits to the smallest acceptable value.
// ResetOnOverflow - when slot reaches its reuse limit it is excluded by default. Setting this
//                   to true overrides this behaviour and removes the element allocation limit,
//                   but in a cost of a possibility of multiple identical handles to exist.

namespace Data
{

template<typename T, typename H = uint32_t, size_t IndexBits = 20, bool ResetOnOverflow = false>
class CHandleArray
{
	static_assert(std::is_unsigned_v<H> && std::is_integral_v<H>, "CHandleArray > H must be an unsigned integral type");
	static_assert(IndexBits > 0, "CHandleArray > IndexBits must not be 0");
	static_assert((sizeof(H) * 8 - IndexBits) > 1, "CHandleArray > too few reuse bits, try less IndexBits or bigger H");
	static_assert(sizeof(UPTR) * 8 > IndexBits, "CHandleArray > too many index bits, not supported by underlying structure");

protected:

	static constexpr H MAX_CAPACITY = ((1 << IndexBits) - 1);
	static constexpr H INDEX_ALLOCATED = MAX_CAPACITY;
	static constexpr H REUSE_BITS_MASK = (static_cast<H>(-1) << IndexBits);
	static constexpr H INDEX_BITS_MASK = ~REUSE_BITS_MASK;

	struct CHandleRec
	{
		T Value;
		H Handle = 0; // Index bits store the next free slot index or INDEX_ALLOCATED value, _not_ this slot index
	};

	std::vector<CHandleRec> _Records;
	UPTR                    _ElementCount = 0;
	UPTR                    _FirstFreeIndex = MAX_CAPACITY;
	UPTR                    _LastFreeIndex = MAX_CAPACITY;

	bool IsFull() const { return _LastFreeIndex == MAX_CAPACITY; }

	// Links newly added records to a single linked list of free records
	void BuildFreeList(UPTR StartIndex = 0)
	{
		const UPTR Size = _Records.size();
		if (StartIndex >= Size) return;

		if (IsFull())
		{
			// No existing free records, build a new free list
			_FirstFreeIndex = StartIndex;
		}
		else
		{
			// Link to the end of existing free list
			auto& PrevFree = _Records[_LastFreeIndex].Handle;
			PrevFree = (PrevFree & REUSE_BITS_MASK) | StartIndex;
		}

		// Reuse bits of the new record are always zero, only set an index part
		for (auto i = StartIndex; i < Size - 1; ++i)
			_Records[i].Handle = i + 1;

		_LastFreeIndex = Size - 1;
	}

public:

	typedef struct { H Raw; } CHandle; // A new type is required for the type safety

	static constexpr CHandle INVALID_HANDLE = { MAX_CAPACITY };

	CHandleArray() = default;
	CHandleArray(UPTR InitialSize) : _Records(InitialSize) { BuildFreeList(); }
	CHandleArray(UPTR InitialSize, const T& Prototype) : _Records(InitialSize, CHandleRec{ Prototype, 0 }) { BuildFreeList(); }

	CHandle Allocate()
	{
		if (!IsFull())
		{
			// Use a free record

			if (_FirstFreeIndex == _LastFreeIndex)
			{
				// The last free record was used
				_FirstFreeIndex = MAX_CAPACITY;
				_LastFreeIndex = MAX_CAPACITY;
			}
			else
			{
				//!!!fix free record list!
			}

			return { WTF };
		}

		const UPTR Size = _Records.size();
		if (Size < MAX_CAPACITY)
		{
			// All records are used but an index space is not exhausted
			_Records.push_back({ T(), INDEX_ALLOCATED });
			return { Size };
		}

		if (ResetOnOverflow)
		{
			// Scan for records with exhausted reuse count, resurrect the first found.
			// Since reuse count is incremented on release, the record is surely free.
			for (UPTR i = 0; i < Size; ++i)
			{
				//Record
				if ((Record.Handle & REUSE_BITS_MASK) == REUSE_BITS_MASK)
				{
					//
				}
			}
		}

		// Can't allocate a new record
		return INVALID_HANDLE;

		// find first free slot + first invalid one (for ResetOnOverflow)
		// if no free but size is not maximal, grow
		// if ResetOnOverflow and no free slots, resurrect the first invalid slot
		//???store starting index for the search? update on allocate and free
	}

	CHandle  Allocate(const T& Value);
	CHandle  Allocate(T&& Value);
	void     Free(CHandle Handle); //!!!don't add to the free list if reuse exhausted!
	void     ResetExhaustedReuse(); //explicit reset of exhausted reuse counters

	bool IsHandleValid(CHandle Handle) const
	{
		const auto Index = (Handle & INDEX_BITS_MASK);
		return Index < _Records.size() && // Index itself is valid
			(Handle & REUSE_BITS_MASK) == (_Records[Index].Handle & REUSE_BITS_MASK); // Reuse count is the same
	}

	T*       GetValue(CHandle Handle);
	const T* GetValue(CHandle Handle) const;

	constexpr UPTR GetMaxSize() const { return MAX_CAPACITY; }

	//!!!begin, end! not trivial, must skip invalid slots, must stop when _Size of processed elements is reached!
	UPTR size() const { return _ElementCount; }
	bool empty() const { return !_ElementCount; }
};

}
