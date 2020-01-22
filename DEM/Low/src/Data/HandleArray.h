#pragma once
#include <cstdint>

// Handle array combines a growable array and a pool allocator. For element referencing
// it provides persistent handles which are never invalidated, unlike pointers or iterators.
// Handle dereferencing is very fast. Don't store raw pointers obtained through handles.
// NB: free record linked list reuses records evenly. Dynamic growing keeps even highly
// dynamic collections from spreading across the memory, if initial size is not very big.
// All records are initialized with a prototype or T() by default.
// Template parameters:
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
	static_assert(sizeof(size_t) * 8 > IndexBits, "CHandleArray > too many index bits, not supported by underlying structure");

public:

	static constexpr H MAX_CAPACITY = ((1 << IndexBits) - 1);

	typedef struct { H Raw; } CHandle; // A new type is required for the type safety

	static constexpr CHandle INVALID_HANDLE = { MAX_CAPACITY };

protected:

	static constexpr H INDEX_ALLOCATED = MAX_CAPACITY;
	static constexpr H REUSE_BITS_MASK = (static_cast<H>(-1) << IndexBits);
	static constexpr H INDEX_BITS_MASK = ~REUSE_BITS_MASK;

	struct CHandleRec
	{
		T Value;
		H Handle = 0; // Index bits store the next free slot index or INDEX_ALLOCATED value, _not_ this slot index
	};

	std::vector<CHandleRec> _Records;
	size_t                  _ElementCount = 0;
	size_t                  _FirstFreeIndex = MAX_CAPACITY;
	size_t                  _LastFreeIndex = MAX_CAPACITY;
	T                       _Prototype;

	bool IsFull() const { return _LastFreeIndex == MAX_CAPACITY; }

	// Links newly added records to a single linked list of free records
	void BuildFreeList(size_t StartIndex = 0)
	{
		const size_t Size = _Records.size();
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
	//---------------------------------------------------------------------

	H AllocateEmpty()
	{
		if (!IsFull())
		{
			// Use a free record
			auto& Record = _Records[_FirstFreeIndex];
			const auto ReuseBits = (Record.Handle & REUSE_BITS_MASK);
			const H NewHandle = ReuseBits | _FirstFreeIndex;

			if (_FirstFreeIndex == _LastFreeIndex)
			{
				// The last free record was used
				_FirstFreeIndex = MAX_CAPACITY;
				_LastFreeIndex = MAX_CAPACITY;
			}
			else
			{
				// Set the next free index as the first
				_FirstFreeIndex = (Record.Handle & INDEX_BITS_MASK);
			}

			Record.Handle = ReuseBits | INDEX_ALLOCATED;

			return NewHandle;
		}

		const size_t Size = _Records.size();
		if (Size < MAX_CAPACITY)
		{
			// All records are used but an index space is not exhausted
			_Records.push_back({ _Prototype, INDEX_ALLOCATED });
			return Size;
		}

		// Can't allocate a new record
		return MAX_CAPACITY;
	}
	//---------------------------------------------------------------------

public:

	CHandleArray() = default;
	CHandleArray(const T& Prototype) : _Prototype(Prototype) {}

	CHandleArray(size_t InitialSize)
		: _Records(std::min<size_t>(InitialSize, MAX_CAPACITY))
	{
		BuildFreeList();
	}

	CHandleArray(size_t InitialSize, const T& Prototype)
		: _Records(std::min<size_t>(InitialSize, MAX_CAPACITY), CHandleRec{ Prototype, 0 })
		, _Prototype(Prototype)
	{
		BuildFreeList();
	}

	CHandle Allocate() { return { AllocateEmpty() }; }

	CHandle Allocate(T&& Value)
	{
		auto Handle = AllocateEmpty();
		if (Handle != MAX_CAPACITY)
			_Records[Handle & INDEX_BITS_MASK].Value = std::move(Value);
		return { Handle };
	}

	CHandle Allocate(const T& Value)
	{
		auto Handle = AllocateEmpty();
		if (Handle != MAX_CAPACITY)
			_Records[Handle & INDEX_BITS_MASK].Value = Value;
		return { Handle };
	}

	void Free(CHandle Handle)
	{
		//!!!don't add to the free list if reuse exhausted!
		//???or instead of searching in AllocateEmpty, resurrect right here?!

		//if (ResetOnOverflow)
		//{
		//	// Scan for records with exhausted reuse count, resurrect the first one found.
		//	// Since the reuse count is incremented on release, the record is surely free.
		//	for (size_t i = 0; i < Size; ++i)
		//	{
		//		auto& Record = _Records[i];
		//		if ((Record.Handle & REUSE_BITS_MASK) == REUSE_BITS_MASK)
		//		{
		//			Record.Handle = INDEX_ALLOCATED; // Reuse bits are cleared to 0
		//			return i;
		//		}
		//	}
		//}
	}

	// NB: advanced method, increased risk!
	void ResetExhaustedReuse()
	{
		//explicit reset of exhausted reuse counters, rebuild free list on the fly!
	}

	// NB: advanced method, increased risk!
	//SetToHandle(Handle, const T& Value)

	// NB: advanced method, increased risk!
	//SetToHandle(Handle, T&& Value)

	T* GetValue(CHandle Handle)
	{
		const auto Index = (Handle.Raw & INDEX_BITS_MASK);
		if (Index < _Records.size())
		{
			// Check that reuse count is the same and the record is not free
			auto& Record = _Records[Index];
			if (((Handle.Raw & REUSE_BITS_MASK) | INDEX_ALLOCATED) == Record.Handle)
				return &Record.Value;
		}

		return nullptr;
	}

	const T* GetValue(CHandle Handle) const
	{
		const auto Index = (Handle.Raw & INDEX_BITS_MASK);
		if (Index < _Records.size())
		{
			// Check that reuse count is the same and the record is not free
			auto& Record = _Records[Index];
			if (((Handle.Raw & REUSE_BITS_MASK) | INDEX_ALLOCATED) == Record.Handle)
				return &Record.Value;
		}

		return nullptr;
	}

	constexpr size_t GetMaxCapacity() const { return MAX_CAPACITY; }
	size_t           GetCurrentCapacity() const { return _Records.size(); }

	//!!!begin, end! not trivial, must skip invalid slots, must stop when _Size of processed elements is reached!
	size_t           size() const { return _ElementCount; }
	bool             empty() const { return !_ElementCount; }
};

}
