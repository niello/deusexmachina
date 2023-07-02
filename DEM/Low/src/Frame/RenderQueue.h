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
using PRenderQueue = std::unique_ptr<class IRenderQueue>;

// Make the whole queue polymorphic instead of sort key generation for better inlining in the Update() loop
class IRenderQueue
{
public:

	virtual ~IRenderQueue() = default;

	virtual void Add(Render::IRenderable* pRenderable) = 0;
	virtual void Remove(Render::IRenderable* pRenderable) = 0;
	virtual void Update() = 0;
};

// TKeyBuilder must define TKey type and TKey operator()(const Render::IRenderable*) const.
template<typename TKeyBuilder, U32 FilterMask = 0>
class CRenderQueue : public IRenderQueue
{
public:

	using TKey = typename TKeyBuilder::TKey;
	static inline constexpr TKey NO_KEY = INVALID_INDEX_T<TKey>; // Implementation must guarantee that no valid key will be equal to this

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
	virtual void Add(Render::IRenderable* pRenderable) override
	{
		// FIXME: now first update of a renderable happens after adding it to the queue but before updating the queue, so this check is moved to Update()
		//???will be fixed when start adding objects on their first visible. not even necessary to remove on becoming invisible if don't want.
		//don't forget to relax condition in Update for new items, as filter mask will be already checked.
		//if (FilterMask & pRenderable->RenderQueueMask)
			_Queue.push_back({ pRenderable, NO_KEY });
	}

	// Remember a key calculated from the not updated state, so it equal to the key currently in a queue.
	virtual void Remove(Render::IRenderable* pRenderable) override
	{
		if (FilterMask & pRenderable->RenderQueueMask)
			_ToRemove.push_back({ pRenderable, TKeyBuilder{}(std::as_const(pRenderable)) });
	}

	virtual void Update() override
	{
		// Sort removal list for faster matching, see the loop below
		std::sort(_ToRemove.begin(), _ToRemove.end());

		constexpr TKeyBuilder KeyBuilder{};
		auto RemoveStartIt = _ToRemove.begin();
		size_t KeysChanged = (_Queue.size() - _SortedSize);

		// Update or remove existing elements. This part of a queue is sorted by Key on the previous update.
		for (size_t i = 0; i < _SortedSize; ++i)
		{
			auto& Record = _Queue[i];
			const auto PrevKey = Record.Key;

			// Should never fail if the queue is used correctly. Failure means that we try to remove something not added.
			n_assert_dbg(RemoveStartIt == _ToRemove.cend() || RemoveStartIt->Key >= PrevKey);

			// Records to remove are sorted by key, so it is guaranteed that all matching keys can be only in a single range starting at RemoveStartIt
			bool Removed = false;
			for (auto It = RemoveStartIt; It != _ToRemove.cend() && It->Key == PrevKey; ++It)
			{
				if (It->pRenderable == Record.pRenderable)
				{
					// Mark the record for removal and skip a sequence of removed records at the beginning of the list
					Removed = true;
					It->pRenderable = nullptr;
					while (RemoveStartIt != _ToRemove.cend() && !RemoveStartIt->pRenderable) ++RemoveStartIt;
					break;
				}
			}

			//???check current frame's change flags in pRenderable and decide if need to recalc key? or recalc always? can be too slow and worth flag-based optimization?
			// Update sorting key. Objects that stopped matching the queue are marked for removal.
			Record.Key = (!Removed && (FilterMask & Record.pRenderable->RenderQueueMask)) ? KeyBuilder(std::as_const(Record.pRenderable)) : NO_KEY;

			//!!!TODO PERF: profile! Branching cost vs more instructions.
			//if (Record.Key != PrevKey) ++KeysChanged;
			KeysChanged += (Record.Key != PrevKey);
		}

		n_assert_dbg(RemoveStartIt == _ToRemove.cend());

		// No records have changed, wee can skip all further processing
		if (!KeysChanged) return;

		// Calculate keys for added elements. They are already counted in KeysChanged.
		for (size_t i = _SortedSize; i < _Queue.size(); ++i)
		{
			auto& Record = _Queue[i];
			if (FilterMask & Record.pRenderable->RenderQueueMask)
				Record.Key = KeyBuilder(std::as_const(Record.pRenderable));
		}

		// Sort ascending, so that records marked for removal with NO_KEY are moved to the tail
		//!!!TODO PERF:
		//if (KeysChanged >= GeneralPurposeSortThreshold)
		std::sort(_Queue.begin(), _Queue.end()); //???try radix sort?
		//else
		// SortByAlmostSortedAlgorithm()

		// Сut the tail where elements marked for removal with NO_KEY are located after sorting
		auto NoKeyIt = std::lower_bound(_Queue.begin(), _Queue.end(), NO_KEY, [](const auto& Elm, TKey Value) { return Elm.Key < Value; });
		if (NoKeyIt != _Queue.cend() && NoKeyIt->Key == NO_KEY) _Queue.erase(NoKeyIt, _Queue.cend());

		_SortedSize = _Queue.size();

		_ToRemove.clear();
	}
};

}
