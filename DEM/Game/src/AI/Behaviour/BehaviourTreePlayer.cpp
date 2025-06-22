#include "BehaviourTreePlayer.h"
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <Math/Math.h>

namespace DEM::AI
{
constexpr auto BT_PLAYER_MIN_BUFFER_ALIGNMENT = sizeof(void*);

static inline size_t CalcTraversalStackSize(const CBehaviourTreeAsset& Asset)
{
	return Math::CeilToMultiple(Asset.GetMaxDepth() * sizeof(U16), BT_PLAYER_MIN_BUFFER_ALIGNMENT);
}
//---------------------------------------------------------------------

static inline size_t CalcBufferSize(const CBehaviourTreeAsset& Asset)
{
	const auto TraversalStackBytes = CalcTraversalStackSize(Asset);
	return 3 * TraversalStackBytes + Asset.GetMaxInstanceBytes();
}
//---------------------------------------------------------------------

CBehaviourTreePlayer::~CBehaviourTreePlayer()
{
	Stop();
}
//---------------------------------------------------------------------

bool CBehaviourTreePlayer::Start(PBehaviourTreeAsset Asset)
{
	const auto PrevBytes = _Asset ? CalcBufferSize(*_Asset) : 0;

	Stop();

	_Asset = std::move(Asset);
	if (!_Asset) return false;

	// Allocate a single buffer for traversal stacks and node instance data
	const auto NewBytes = CalcBufferSize(*_Asset);
	if (PrevBytes < NewBytes)
	{
		_MemBuffer.reset(new std::byte[NewBytes]);
		n_assert_dbg(IsAligned<BT_PLAYER_MIN_BUFFER_ALIGNMENT>(_MemBuffer.get()));

		const auto TraversalStackBytes = CalcTraversalStackSize(*_Asset);
		_pNewStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get()));
		_pActiveStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get() + TraversalStackBytes));
		_pRequestStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get() + 2 * TraversalStackBytes));
		_pNodeInstanceData = _MemBuffer.get() + 3 * TraversalStackBytes;
	}

	return true;
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::Stop()
{
	// unwind current subtree to the root, cancelling actions and destructing instance data structures, but don't deallocate buffers
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::Update(Game::CGameSession& Session, float dt)
{
	if (!_Asset) return;

	// Start from the root
	_pNewStack[0] = 0;
	EBTStatus Status = EBTStatus::Running;
	U16 PrevIdx = 0;
	U16 NextIdx = 0;
	U16 CurrIdx = 0;
	U16 NewLevel = 0;

	//!!!TODO: pack context into a single structure? or only mutable part? or immutable too, as a second struct?

	while (true)
	{
		const bool IsGoingDown = (CurrIdx >= PrevIdx);
		const auto* pNode = _Asset->GetNode(CurrIdx);

		// If traversing down and following the active path
		if (IsGoingDown && NewLevel < _ActiveDepth && CurrIdx == _pActiveStack[NewLevel])
		{
			// Update an already active node
			std::tie(Status, NextIdx) = pNode->pNodeImpl->Update(CurrIdx, dt);

			// The most often case for the node is to request itself when explicit traversal change is not needed
			if (NextIdx == CurrIdx)
			{
				// If no children, return to parent. Else proceed to the active child node or overriding high priority request.
				const auto NextLevel = NewLevel + 1;
				NextIdx = (NextLevel == _ActiveDepth) ? pNode->SkipSubtreeIndex : _pRequestStack[NextLevel];
			}
		}
		else
		{
			// Search for a new active path in the tree.
			// NB: Status value makes sense only for upwards traversal, otherwise the node is considered Running.
			if (IsGoingDown)
				std::tie(Status, NextIdx) = pNode->pNodeImpl->TraverseFromParent(CurrIdx, pNode->SkipSubtreeIndex, Session);
			else
				std::tie(Status, NextIdx) = pNode->pNodeImpl->TraverseFromChild(CurrIdx, pNode->SkipSubtreeIndex, NextIdx, Status, Session);

			// If the node requests itself, it is the new active node
			if (NextIdx == CurrIdx)
			{
				// When we have no active subtree we will activate nodes starting from the root
				U16 Level = 0;

				// Deactivate the previously active subtree up to common parent with the new one.
				// NB: both stacks always start from the root so [0] is always identical and Level won't become negative.
				if (_ActiveDepth)
				{
					Level = _ActiveDepth - 1;
					while (Level > NewLevel || _pActiveStack[Level] != _pNewStack[Level])
					{
						_Asset->GetNode(_pActiveStack[Level])->pNodeImpl->Deactivate();
						--Level;
					}

					// Proceed from the common parent to the first child on the new path (if any)
					++Level;
				}

				// Activate the new subtree
				while (Level <= NewLevel)
				{
					const auto ActivatingIdx = _pNewStack[Level];
					Status = _Asset->GetNode(ActivatingIdx)->pNodeImpl->Activate();

					//???what if succeeded? or instead of status activation must return only bool?
					if (Status == EBTStatus::Failed)
					{
						// Rollback to the last successfully activated node and stop activation
						NewLevel = Level - 1;
						pNode = _Asset->GetNode(_pActiveStack[NewLevel]);
						break;
					}

					_pActiveStack[Level++] = ActivatingIdx;
				}

				// "Level" is now 1 level past the last activated node
				_ActiveDepth = Level;

				// A new subtree was activated and the status must be returned to the active node's parent regardless of the value.
				// NB: an active node can't currently proceed down the tree, it sould have done that in Traverse. May change later.
				NextIdx = pNode->SkipSubtreeIndex;
			}
		}

		n_assert_dbg(NextIdx > CurrIdx && NextIdx <= pNode->SkipSubtreeIndex);

		PrevIdx = CurrIdx;

		if (NextIdx >= pNode->SkipSubtreeIndex)
		{
			// Subtree is finished, must return to the parent
			if (!NewLevel) break;
			CurrIdx = _pNewStack[--NewLevel];
		}
		else
		{
			// Proceed to the child
			_pNewStack[++NewLevel] = NextIdx;
			CurrIdx = NextIdx;
		}
	}

	// Initially we have no higher priority requests and default to continuing along the current active path
	std::copy_n(_pActiveStack, _ActiveDepth, _pRequestStack);

	//!!!must check that after failed activation and rollback the newly activated node won't update on the next iteration!

	// if Status = failed here does it always mean that non-empty current stack was failed to activate and must be aborted to the root?!

	// at the end Status contains the root status, can detect tree execution end to stop or loop!

	//???does that all mean that any non-running status here must deactivate an active tree?!
	//???or do it externally where the player update is called, based on the return walue? should really be there or better to incapsulate it here?
}
//---------------------------------------------------------------------

}
