#pragma once
#include <Game/ECS/Entity.h>
#include <System/Allocators/PoolAllocator.h>

// Specialized hash map for entity -> component index.
// Only required subset of operations is implemented. No iterator provided. To iterate
// over entities with a component, use an iterator from the component storage.

// TODO: use lock-free pool for records

namespace DEM::Game
{

template<class T>
class CEntityMap final
{
public:

	struct CRecord
	{
		CRecord* pPrev = nullptr;
		CRecord* pNext = nullptr;
		HEntity  EntityID;
		T        Value;
	};

private:

	static constexpr size_t MIN_BUCKETS = 8;
	static constexpr size_t MAX_BUCKETS = std::numeric_limits<size_t>().max() / 16;
	static constexpr float MAX_LOAD_FACTOR = 1.5f;

	// FIXME: thread safety! can use lock-free pool.
	// FIXME: review my old pool allocator, maybe less effective than modern ones.
	static inline CPool<CRecord, 1024> _Pool;

	std::vector<CRecord*> _Records;
	size_t                _Size = 0;
	size_t                _HashMask;

	void Rehash(size_t BucketCount)
	{
		BucketCount = NextPow2(std::clamp(BucketCount, MIN_BUCKETS, MAX_BUCKETS));
		if (BucketCount == _Records.size()) return;

		std::vector<CRecord*> OldRecords;
		std::swap(OldRecords, _Records);

		_Records.resize(BucketCount, nullptr);

		// Buckets are always pow2, so (hash & (bucket count - 1)) gives the bucket index like % operator.
		// It is guaranteed that all valid entities have different indices, which are more or less evenly
		// distributed, so we use index bits as a hash value and incorporate their mask here.
		_HashMask = ((BucketCount - 1) & CEntityStorage::INDEX_BITS_MASK);

		for (auto* pRecord : OldRecords)
		{
			while (pRecord)
			{
				auto pNext = pRecord->pNext;

				auto& Bucket = _Records[pRecord->EntityID.Raw & _HashMask];
				pRecord->pPrev = nullptr;
				pRecord->pNext = Bucket;
				if (Bucket) Bucket->pPrev = pRecord;
				Bucket = pRecord;

				pRecord = pNext;
			}
		}
	}

public:

	class iterator
	{
	private:

		using TBucket = typename std::vector<CRecord*>::iterator;

		TBucket _BucketIt;
		TBucket _EndIt;
		CRecord* _pRecord = nullptr;

		friend class CEntityMap;

	public:

		using iterator_category = std::forward_iterator_tag;

		using value_type = T;
		using difference_type = ptrdiff_t;
		using pointer = T*;
		using reference = T&;

		iterator() = default;
		iterator(TBucket BucketIt, TBucket EndIt, CRecord* pRecord) : _BucketIt(BucketIt), _EndIt(EndIt), _pRecord(pRecord) {}
		iterator(const iterator& It) = default;
		iterator& operator =(const iterator& It) = default;

		T&   operator *() const { return _pRecord->Value; }
		T*   operator ->() const { return _pRecord ? &_pRecord->Value : nullptr; }
		bool operator ==(const iterator Other) const { return _pRecord == Other._pRecord; }
		bool operator !=(const iterator Other) const { return _pRecord != Other._pRecord; }

		iterator operator ++(int) { auto Tmp = *this; ++(*this); return Tmp; }
		iterator& operator ++()
		{
			if (_pRecord)
			{
				if (_pRecord->pNext) _pRecord = _pRecord->pNext;
				else
				{
					++_BucketIt;
					if (_BucketIt == _EndIt) _pRecord = nullptr;
					else _pRecord = *_BucketIt;
				}
			}

			return *this;
		}
	};

	// FIXME: define template and specialize for T and const T?
	using const_iterator = const iterator;

	CEntityMap(size_t BucketCount = 0)
	{
		Rehash(BucketCount);
	}

	~CEntityMap()
	{
		clear();
	}

	void emplace(HEntity EntityID, T&& Value)
	{
		n_assert_dbg(EntityID);

		auto BucketIndex = (EntityID.Raw & _HashMask);

		// Try to find existing record in the bucket and replace its value
		auto pRecord = _Records[BucketIndex];
		while (pRecord)
		{
			if (pRecord->EntityID == EntityID)
			{
				pRecord->Value = std::move(Value);
				return;
			}

			pRecord = pRecord->pNext;
		}

		// If this record makes load factor too high, rehash the table and update bucket index
		const auto BucketCount = _Records.size();
		const float LoadFactor = (_Size + 1) / static_cast<float>(BucketCount);
		if (LoadFactor > MAX_LOAD_FACTOR)
		{
			// Grow faster at the beginning
			Rehash(BucketCount * (BucketCount < 2048 ? 8 : 2));
			BucketIndex = (EntityID.Raw & _HashMask);
		}

		// Add new record to the bucket
		auto& Bucket = _Records[BucketIndex];
		auto pNewRecord = _Pool.Construct(CRecord{ nullptr, Bucket, EntityID, std::move(Value) });
		if (Bucket) Bucket->pPrev = pNewRecord;
		Bucket = pNewRecord;
		++_Size;
	}

	CRecord* find(HEntity EntityID)
	{
		auto pRecord = _Records[EntityID.Raw & _HashMask];
		while (pRecord)
		{
			if (pRecord->EntityID == EntityID) return pRecord;
			pRecord = pRecord->pNext;
		}
		return nullptr;
	}

	const CRecord* find(HEntity EntityID) const
	{
		auto pRecord = _Records[EntityID.Raw & _HashMask];
		while (pRecord)
		{
			if (pRecord->EntityID == EntityID) return pRecord;
			pRecord = pRecord->pNext;
		}
		return nullptr;
	}

	void erase(CRecord* pRecord)
	{
		if (!pRecord) return;

		if (pRecord->pPrev)
			pRecord->pPrev->pNext = pRecord->pNext;
		else
			_Records[pRecord->EntityID.Raw & _HashMask] = pRecord->pNext;

		if (pRecord->pNext) pRecord->pNext->pPrev = pRecord->pPrev;

		_Pool.Destroy(pRecord);
		--_Size;
	}

	void clear()
	{
		for (CRecord* pRecord : _Records)
		{
			while (pRecord)
			{
				auto pDestroyMe = pRecord;
				pRecord = pRecord->pNext;
				_Pool.Destroy(pDestroyMe);
			}
		}
	}

	template<typename TCallback>
	void ForEach(TCallback Callback)
	{
		for (CRecord* pRecord : _Records)
		{
			while (pRecord)
			{
				Callback(pRecord->EntityID, pRecord->Value);
				pRecord = pRecord->pNext;
			}
		}
	}

	template<typename TCallback>
	void ForEach(TCallback Callback) const
	{
		for (CRecord* pRecord : _Records)
		{
			while (pRecord)
			{
				Callback(pRecord->EntityID, pRecord->Value);
				pRecord = pRecord->pNext;
			}
		}
	}

	iterator       begin() { return iterator{ _Records.begin(), _Records.end(), _Records.front() };  }
	iterator       end() { return iterator(); }
	const_iterator end() const { return cend(); }
	const_iterator cbegin() { return iterator{ _Records.begin(), _Records.end(), _Records.front() };  }
	const_iterator cend() const { return iterator(); }
	size_t         size() const { return _Size; }
	bool           empty() const { return !_Size; }
};

}
