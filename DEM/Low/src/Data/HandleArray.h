#pragma once
#include <type_traits>
#include <vector>

// Handle array combines a growable array and a pool allocator. For element referencing
// it provides persistent handles which are never invalidated, unlike pointers or iterators.
// Handle dereferencing is very fast, so try to avoid storing raw pointers obtained through handles.
// Records are reused mostly evenly. Dynamic growing keeps even highly dynamic collections
// from spreading across the memory, if initial size is not very big.
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
	static_assert(std::is_unsigned_v<H> && std::is_integral_v<H> && !std::is_same_v<H, bool>,
		"CHandleArray > H must be an unsigned integral type other than bool");
	static_assert(IndexBits > 0, "CHandleArray > IndexBits must not be 0");
	static_assert((sizeof(H) * 8 - IndexBits) > 1, "CHandleArray > too few reuse bits, try less IndexBits or bigger H");
	static_assert(sizeof(size_t) * 8 > IndexBits, "CHandleArray > too many index bits, not supported by underlying structure");

public:

	using THandleValue = H;

	static constexpr H MAX_CAPACITY = ((1 << IndexBits) - 1);
	static constexpr H INDEX_ALLOCATED = MAX_CAPACITY;
	static constexpr H REUSE_BITS_MASK = (static_cast<H>(-1) << IndexBits);
	static constexpr H INDEX_BITS_MASK = ~REUSE_BITS_MASK;
	static constexpr H INVALID_HANDLE_VALUE = MAX_CAPACITY;

	// A new type is required for type safety
	struct CHandle
	{
		H Raw = INVALID_HANDLE_VALUE;

		bool operator <(CHandle Other) const { return Raw < Other.Raw; }
		bool operator ==(CHandle Other) const { return Raw == Other.Raw; }
		bool operator !=(CHandle Other) const { return Raw != Other.Raw; }
		operator bool() const { return Raw != INVALID_HANDLE_VALUE; }
		operator H() const { return Raw; }
	};

	static constexpr CHandle INVALID_HANDLE = { INVALID_HANDLE_VALUE };

protected:

	struct CHandleRec
	{
		T Value;
		H HandleData = 0; // Index bits store the next free slot index or INDEX_ALLOCATED value, _not_ this slot index
	};

	std::vector<CHandleRec> _Records;
	size_t                  _ElementCount = 0;
	size_t                  _FirstFreeIndex = MAX_CAPACITY;
	size_t                  _LastFreeIndex = MAX_CAPACITY;
	const T                 _Prototype;

	bool IsFull() const { return _LastFreeIndex == MAX_CAPACITY; }

	// Links newly added records to a single linked list of free records
	void AddRangeToFreeList(size_t StartIndex, size_t EndIndex)
	{
		if (IsFull())
		{
			// No existing free records, build a new free list
			_FirstFreeIndex = StartIndex;
		}
		else
		{
			// Link to the end of existing free list
			auto& PrevFree = _Records[_LastFreeIndex].HandleData;
			PrevFree = (PrevFree & REUSE_BITS_MASK) | StartIndex;
		}

		// Keep reuse bits intact
		for (auto i = StartIndex; i < EndIndex; ++i)
			_Records[i].HandleData = (_Records[i].HandleData & REUSE_BITS_MASK) | (i + 1);

		_LastFreeIndex = EndIndex;
	}
	//---------------------------------------------------------------------

	void ConsumeFirstFreeRecord()
	{
		auto& HandleData = _Records[_FirstFreeIndex].HandleData;

		if (_FirstFreeIndex == _LastFreeIndex)
		{
			// The last free record was used
			_FirstFreeIndex = MAX_CAPACITY;
			_LastFreeIndex = MAX_CAPACITY;
		}
		else
		{
			// Set the next free index as the first
			_FirstFreeIndex = (HandleData & INDEX_BITS_MASK);
		}

		HandleData |= INDEX_ALLOCATED;
	}
	//---------------------------------------------------------------------

	H AllocateEmpty()
	{
		if (!IsFull())
		{
			// Use a free record
			const H NewHandle = (_Records[_FirstFreeIndex].HandleData & REUSE_BITS_MASK) | _FirstFreeIndex;
			ConsumeFirstFreeRecord();
			++_ElementCount;
			return NewHandle;
		}

		const size_t Size = _Records.size();
		if (Size < MAX_CAPACITY)
		{
			// All records are used but an index space is not exhausted
			if constexpr (std::is_copy_constructible_v<T>)
				_Records.push_back({ _Prototype, INDEX_ALLOCATED });
			else
				_Records.push_back({ {}, INDEX_ALLOCATED });
			++_ElementCount;
			return Size;
		}

		// Can't allocate a new record
		return INVALID_HANDLE_VALUE;
	}
	//---------------------------------------------------------------------

	bool AllocateEmptyAt(H Handle)
	{
		// Can't add a handle with exhausted reuse counter
		if ((Handle & REUSE_BITS_MASK) == REUSE_BITS_MASK) return false;

		const auto Index = (Handle & INDEX_BITS_MASK);
		if (Index >= MAX_CAPACITY) return false;

		const size_t Size = _Records.size();
		if (Index >= Size)
		{
			// Record[Index] is immediately allocated, all other added records are attached to the free list
			if constexpr (std::is_copy_constructible_v<T>)
				_Records.resize(Index + 1, { _Prototype, 0 } );
			else
				_Records.resize(Index + 1);
			if (Index > Size)
				AddRangeToFreeList(Size, Index - 1);
		}
		else
		{
			// Fail if target record is already allocated
			if ((_Records[Index].HandleData & INDEX_BITS_MASK) == INDEX_ALLOCATED) return false;

			// Fixup the free list. It is not empty because at least our target record is free.
			if (Index == _FirstFreeIndex)
			{
				// Our record is the first, do standard allocation logic
				ConsumeFirstFreeRecord();
			}
			else
			{
				// Our record is not first. Search for it, exclude and fix links.
				auto PrevIndex = _FirstFreeIndex;
				do
				{
					auto& PrevHandle = _Records[PrevIndex].HandleData;
					auto CurrIndex = (PrevHandle & INDEX_BITS_MASK);

					if (CurrIndex == Index)
					{
						if (CurrIndex == _LastFreeIndex)
						{
							// Our record was last, fixing list end is enough
							_LastFreeIndex = PrevIndex;
						}
						else
						{
							// Link previous record to the next one, skipping current
							PrevHandle = (PrevHandle & REUSE_BITS_MASK) | (_Records[CurrIndex].HandleData & INDEX_BITS_MASK);
						}

						break;
					}

					PrevIndex = CurrIndex;
				}
				while (PrevIndex != _LastFreeIndex); // Must never happen, 'true' would suffice too
			}
		}

		// Get reuse counter from the specified handle and mark the record allocated
		_Records[Index].HandleData = (Handle | INDEX_ALLOCATED);

		++_ElementCount;
		return true;
	}
	//---------------------------------------------------------------------

public:

	CHandleArray() = default;
	CHandleArray(const T& Prototype) : _Prototype(Prototype)
	{
		static_assert(std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>, "Using prototype requires copyable T");
	}

	CHandleArray(size_t InitialSize)
		: _Records(std::min<size_t>(InitialSize, MAX_CAPACITY))
	{
		if (auto Size = _Records.size())
			AddRangeToFreeList(0, Size - 1);
	}

	CHandleArray(size_t InitialSize, const T& Prototype)
		: _Records(std::min<size_t>(InitialSize, MAX_CAPACITY), CHandleRec{ Prototype, 0 })
		, _Prototype(Prototype)
	{
		static_assert(std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>, "Using prototype requires copyable T");

		if (auto Size = _Records.size())
			AddRangeToFreeList(0, Size - 1);
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

	bool Free(CHandle Handle)
	{
		const auto Index = (Handle.Raw & INDEX_BITS_MASK);
		if (Index >= _Records.size()) return false;

		auto& Record = _Records[Index];
		if ((Handle.Raw | INDEX_ALLOCATED) != Record.HandleData) return false;

		// Clear the record value to default, this clears freed object resources
		if constexpr (std::is_copy_assignable_v<T>)
			Record.Value = _Prototype;

		--_ElementCount;

		// Reuse counter of freed record is incremented. Index bits are reset to 0
		// to clear INDEX_ALLOCATED. Existing handles immediately become invalid.
		const auto ReuseBits = (Record.HandleData & REUSE_BITS_MASK);
		constexpr H LAST_REUSE_VALUE = (REUSE_BITS_MASK - (1 << IndexBits));
		if (ReuseBits == LAST_REUSE_VALUE)
		{
			if constexpr (ResetOnOverflow)
			{
				// Clear reuse counter. The record is considered free again, but a
				// very old handle may exist that now becomes incorrectly "valid".
				// A probability of this is very low in practice.
				Record.HandleData = 0;
			}
			else
			{
				// Don't add this record to the free list. Mark as exhausted.
				Record.HandleData = REUSE_BITS_MASK;
				return true;
			}
		}
		else
		{
			Record.HandleData = ReuseBits + (1 << IndexBits);
		}

		AddRangeToFreeList(Index, Index);
		return true;
	}

	// Deletes unused records from the tail of the inner storage. Doesn't invalidate handles but
	// leads to loss of reuse counters, so growing may lead to "validation" of expired handles.
	// NB: advanced method, increased risk!
	void ShrinkToFit()
	{
		// Find first allocated record from the end
		auto Rit = _Records.crbegin();
		for (; Rit != _Records.crend(); ++Rit)
			if ((Rit->HandleData & INDEX_BITS_MASK) == INDEX_ALLOCATED) break;

		// base() returns us the first record to delete
		auto It = Rit.base();
		if (It != _Records.end())
		{
			_Records.erase(It, _Records.end());

			// Rebuild the free list from scratch, it is easier than repairing it for all possible cases
			_FirstFreeIndex = MAX_CAPACITY;
			_LastFreeIndex = MAX_CAPACITY;

			const size_t Size = _Records.size();
			for (size_t i = 0; i < Size; ++i)
			{
				// Skip allocated and exhausted records, process free only
				auto& HandleData = _Records[i].HandleData;
				if ((HandleData & REUSE_BITS_MASK) == REUSE_BITS_MASK) continue;
				if ((HandleData & INDEX_BITS_MASK) == INDEX_ALLOCATED) continue;

				AddRangeToFreeList(i, i);
			}
		}

		// Finally free unused memory even if no records were deleted
		_Records.shrink_to_fit();
	}

	// Explicit reset of exhausted reuse counters. The effect is the same as from ResetOnOverflow = true.
	// Call when you're sure that no expired handles exist in a client code that can become "valid".
	// NB: advanced method, increased risk!
	void ResetExhaustedReuse()
	{
		const size_t Size = _Records.size();
		for (size_t i = 0; i < Size; ++i)
		{
			auto& HandleData = _Records[i].HandleData;
			if ((HandleData & REUSE_BITS_MASK) != REUSE_BITS_MASK) continue;

			HandleData &= (~REUSE_BITS_MASK);
			AddRangeToFreeList(i, i);
		}
	}

	// Store the value with the specified handle. Useful for serialization and replication.
	// Fails if target record is already allocated.
	// NB: advanced method, increased risk!
	CHandle AllocateWithHandle(H Handle, const T& Value)
	{
		if (!AllocateEmptyAt(Handle)) return { INVALID_HANDLE_VALUE };
		_Records[Handle & INDEX_BITS_MASK].Value = Value;
		return { Handle };
	}

	// Store the value with the specified handle. Useful for serialization and replication.
	// Fails if target record is already allocated.
	// NB: advanced method, increased risk!
	CHandle AllocateWithHandle(H Handle, T&& Value)
	{
		if (!AllocateEmptyAt(Handle)) return { INVALID_HANDLE_VALUE };
		_Records[Handle & INDEX_BITS_MASK].Value = std::move(Value);
		return { Handle };
	}

	void Clear(size_t NewInitialSize = 0)
	{
		_ElementCount = 0;
		_FirstFreeIndex = MAX_CAPACITY;
		_LastFreeIndex = MAX_CAPACITY;

		if constexpr (std::is_copy_constructible_v<T>)
			_Records.resize(std::min<size_t>(NewInitialSize, MAX_CAPACITY), { _Prototype, 0 });
		else
			_Records.resize(std::min<size_t>(NewInitialSize, MAX_CAPACITY));

		if (auto Size = _Records.size())
		{
			AddRangeToFreeList(0, Size - 1);
			_Records.back().HandleData = 0;
		}
	}

	// Returns a handle by value pointer. Inversion of GetValue. Unsafe, because the value pointer is not
	// guaranteed to be pointing to the same object as at the creation time. To detect actual value change,
	// user must store previous handle and compare with returned one.
	// NB: advanced method, increased risk!
	CHandle GetHandle(const T* pValue) const
	{
		// Slightly hacky, but O(1) is O(1). Uses the fact that pValue has a handle only if it is stored
		// in _Records[index].Value. Record is obtained from value with the structure member offset, and
		// then is checked against _Records address range to calculate the index.
		auto pRecord = reinterpret_cast<const CHandleRec*>(reinterpret_cast<size_t>(pValue) - offsetof(CHandleRec, Value));
		auto Index = static_cast<size_t>(pRecord - _Records.data());
		if (Index >= _Records.size()) return INVALID_HANDLE;
		if ((pRecord->HandleData & INDEX_BITS_MASK) != INDEX_ALLOCATED) return INVALID_HANDLE;
		const auto ReuseBits = (pRecord->HandleData & REUSE_BITS_MASK);
		return (ReuseBits == REUSE_BITS_MASK) ? INVALID_HANDLE : CHandle{ ReuseBits | Index };
	}

	const T* GetValue(CHandle Handle) const
	{
		const auto Index = (Handle.Raw & INDEX_BITS_MASK);
		const bool IsValid = (Index < _Records.size()) && ((Handle.Raw | INDEX_ALLOCATED) == _Records[Index].HandleData);
		return IsValid ? &_Records[Index].Value : nullptr;
	}

	T*               GetValue(CHandle Handle) { return const_cast<T*>(const_cast<const CHandleArray*>(this)->GetValue(Handle)); }
	constexpr size_t GetMaxCapacity() const { return MAX_CAPACITY; }
	size_t           GetCurrentCapacity() const { return _Records.size(); }

	// Bypasses all validity checks. Useful for performance-critical code with external validity guarantee.
	// NB: advanced method, increased risk!
	const T*         GetValueUnsafe(CHandle Handle) const { return &_Records[Handle.Raw & INDEX_BITS_MASK].Value; }
	T*               GetValueUnsafe(CHandle Handle) { return &_Records[Handle.Raw & INDEX_BITS_MASK].Value; }

// *** STL part ***

protected:

	// Iterates through allocated records only
	template<typename internal_it>
	class iterator_tpl
	{
	private:

		std::vector<CHandleRec>& _Storage;
		internal_it              _It;

	public:

		using iterator_category = std::bidirectional_iterator_tag;

		using value_type = typename internal_it::value_type;
		using difference_type = typename internal_it::difference_type;
		using pointer = value_type*;
		using reference = value_type&;

		iterator_tpl() = default;
		iterator_tpl(std::vector<CHandleRec> Storage, internal_it It)
			: _Storage(Storage)
			, _It(It)
		{
			while (_It != _Storage.end() && !IsAllocated()) ++_It;
			//UpdatePair();
		}
		iterator_tpl(const iterator_tpl& It) = default;
		iterator_tpl& operator =(const iterator_tpl& It) = default;

		auto& operator *() const { return _It->Value; }
		auto* operator ->() const { return &_It->Value; }
		iterator_tpl& operator ++() { do ++_It; while (_It != _Storage.end() && !IsAllocated()); return *this; }
		iterator_tpl operator ++(int) { auto Tmp = *this; ++(*this); return Tmp; }
		iterator_tpl& operator --() { do --_It; while (_It != _Storage.end() && !IsAllocated()); return *this; }
		iterator_tpl operator --(int) { auto Tmp = *this; --(*this); return Tmp; }

		bool operator ==(const iterator_tpl& Right) const { return _It == Right._It; }
		bool operator !=(const iterator_tpl& Right) const { return _It != Right._It; }

		bool IsAllocated() const { return (_It->Handle & INDEX_BITS_MASK) == INDEX_ALLOCATED; }
	};

public:

	using iterator = iterator_tpl<typename std::vector<CHandleRec>::iterator>;
	using const_iterator = iterator_tpl<typename std::vector<CHandleRec>::const_iterator>;

	const_iterator   cbegin() const { return const_iterator(_Records, _Records.cbegin()); }
	iterator         begin() { return iterator(_Records, _Records.begin()); }
	const_iterator   begin() const { return cbegin(); }
	const_iterator   cend() const { return const_iterator(_Records, _Records.cend()); }
	iterator         end() { return iterator(_Records, _Records.end()); }
	const_iterator   end() const { return cend(); }
	size_t           size() const { return _ElementCount; }
	bool             empty() const { return !_ElementCount; }
};

}
