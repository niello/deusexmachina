#pragma once
#include <StdDEM.h>

// A rendering queue is a set of renderables filtered and sorted according to certain rules.
// Different filters and sortings are used for draw call and state change optinizations
// in different frame rendering phases.

namespace Render
{
	class IRenderable;
}

namespace Frame
{
//using PRenderQueue = Ptr<class CRenderQueue>; // std::unique_ptr

//!!!DBG TMP!
struct CDummyKey32
{
	using TKey = U32;

	TKey operator()(Render::IRenderable* /*pRenderable*/) const { return 0; }
};

//???on update, check filter only at records up to _SortedSize? next ones are just added and therefore already filtered by insertion code.
//???use CRTP to inject templated key generation into the shared algorithm?
template<typename TKeyBuilder = CDummyKey32, U32 FilterMask = 0>
class CRenderQueue
{
public:

	using TKey = typename TKeyBuilder::TKey;
	static inline constexpr TKey NO_KEY = INVALID_INDEX_T<TKey>; // Key builder guarantees that no valid key will be equal to this

	struct CRecord
	{
		Render::IRenderable* pRenderable;
		TKey                 Key;

		bool operator <(const CRecord& Other) const { return Key < Other.Key; }
	};

	std::vector<CRecord> _Queue;
	std::vector<CRecord> _ToRemove;
	size_t               _SortedSize = 0;

	// Add to the end of the queue with an empty key. It will be calculated on update, just before sorting.
	void Add(Render::IRenderable* pRenderable)
	{
		if (FilterMask & pRenderable->RenderQueueMask)
			_Queue.push_back({ pRenderable, NO_KEY });
	}

	// Remember a key calculated from the not updated state, so it equal to the key currently in a queue.
	void Remove(Render::IRenderable* pRenderable)
	{
		if (FilterMask & pRenderable->RenderQueueMask)
			_ToRemove.push_back({ pRenderable, TKeyBuilder{}(pRenderable) });
	}

	void Update()
	{
		std::sort(_ToRemove.begin(), _ToRemove.end());

		constexpr TKeyBuilder KeyBuilder{};
		auto RemoveStartIt = _ToRemove.begin();
		size_t KeysChanged = (_Queue.size() - _SortedSize) + _ToRemove.size();

		// Update or remove existing elements. This part of a queue is sorted by Key on the previous update.
		for (size_t i = 0; i < _SortedSize; ++i)
		{
			auto& Record = _Queue[i];

			// Should never fail if the queue is used correctly. Failure means that we try to remove something not added.
			n_assert_dbg(RemoveStartIt == _ToRemove.cend() || RemoveStartIt->Key >= Record.Key);

			//!!!!!!!!RemoveIt must be reset to range start until the key has changed!
			//if remove the first record, can advance 1 rec as optimization!

			auto RemoveIt = RemoveStartIt;

			// Records to remove are sorted by key, so it is guaranteed that all matching keys can be only in a single range starting at RemoveIt
			while (RemoveIt != _ToRemove.cend() && RemoveIt->Key == Record.Key)
			{
				if (RemoveIt->pRenderable == Record.pRenderable)
				{
					// Mark the record for removal
					if (RemoveIt == RemoveStartIt) ++RemoveStartIt;
					Record.Key = NO_KEY;
					break;
				}

				++RemoveIt;
			}

			//!!!if removed, must skip update! now removed ones leak here!!!
			//???move removal to a separate function?! it is not templated and it is long enough!

			//???check current frame's change flags in pRenderable and decide if need to recalc key? or recalc always? can be too slow and worth flag-based optimization?

			// Update sorting key. Objects that stopped matching the queue are marked for removal.
			const auto PrevKey = Record.Key;
			Record.Key = (FilterMask & Record.pRenderable->RenderQueueMask) ? KeyBuilder(Record.pRenderable) : NO_KEY;

			// TODO PERF: profile! Branching cost vs more instructions.
			//if (Record.Key != PrevKey) ++KeysChanged;
			KeysChanged += (Record.Key != PrevKey);
		}

		// Setup added elements
		for (size_t i = _SortedSize; i < _Queue.size(); ++i)
			_Queue[i].Key = KeyBuilder(_Queue[i].pRenderable);

		// Sort ascending, so that records marked for removal with NO_KEY are moved to the tail
		if (KeysChanged)
		{
			//!!!TODO:
			//if (KeysChanged >= GeneralPurposeSortThreshold)
			std::sort(_Queue.begin(), _Queue.end());
			//else
			// SortByAlmostSortedAlgorithm()
		}

		// cut the tail with NO_KEY (or calc number of deleted records? O(1) tail search instead of O(log n)!
		//???or one search for lower_bound of NO_KEY is better than calculating all NO_KEY records during iteration?!
		//???use resize instead of erase range?
		auto NoKeyIt = std::lower_bound(_Queue.begin(), _Queue.end(), NO_KEY, [](const auto& Elm, TKey Value) { return Elm.Key < Value; });
		if (NoKeyIt != _Queue.cend()) _Queue.erase(NoKeyIt, _Queue.cend());

		_SortedSize = _Queue.size();

		n_assert_dbg(RemoveStartIt == _ToRemove.cend());
		_ToRemove.clear();
	}

	//???!!!sort only by key or by pair? erase needs to check not only key because it is not unique!!! but maybe checking more records on erase is OK
	// instead of making the whole sorting more expensive.
};

}
