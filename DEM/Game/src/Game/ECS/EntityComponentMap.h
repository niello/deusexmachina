#pragma once
#include <System/Allocators/PoolAllocator.h>

// Specialized hash map for entity -> component index.
// Only required subset of operations is implemented. No iterator provided. To iterate
// over entities with a component, use an iterator from the component storage.

// TODO: use lock-free pool for records

namespace DEM::Game
{

template<class T>
class CEntityComponentMap
{
private:

	static constexpr size_t MIN_BUCKETS = 8;
	static constexpr size_t MAX_BUCKETS = std::numeric_limits<size_t>().max() / 16; //!!!must be pow2!
	static constexpr float MAX_LOAD_FACTOR = 1.5f;

	struct CRecord
	{
		CRecord* pNext = nullptr;
		HEntity  EntityID;
		T        Handle;
	};

	// FIXME: thread safety! can use lock-free pool.
	// FIXME: review my old pool allocator, maybe less effective than modern ones.
	static CPoolAllocator<CRecord, 1024> _Pool;

	std::vector<CRecord*> _Records;
	size_t                _Size = 0;
	size_t                _HashMask; // Bucket count - 1. Buckets are always pow2, and applying this mask to the hash gives the bucket index.

	//void Rehash(new bucket count, but must be next pow2 and not more than max buckets)

public:

	CEntityComponentMap(size_t BucketCount = 0) : _Records(std::min(BucketCount, MIN_BUCKETS), nullptr) { _HashMask = _Records.size() - 1; }

	void emplace(HEntity EntityID, T ComponentHandle)
	{
		// check if already exists, replace value and return prev one if required

		// check total count, rehash if too many, load factor is elements/buckets
		const float LoadFactor = (_Size + 1) / _Records.size();
		//if (_Size + 1 < )
		//{
		//}
	}

	//iterator       find(HEntity EntityID);
	//const_iterator cend();
	//void           emplace(HEntity EntityID, T ComponentHandle);
	//void           erase(iterator It);
};

}
