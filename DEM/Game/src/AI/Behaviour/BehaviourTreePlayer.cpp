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
		_pCurrStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get() + TraversalStackBytes));
		_pRequestStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get() + 2 * TraversalStackBytes));
		_pNodeInstanceData = _MemBuffer.get() + 3 * TraversalStackBytes;
	}

	return true;
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::Stop()
{
	// unwind current subtree to the root, cancelling actions, but don't deallocate buffers
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::Update(Game::CGameSession& Session, float dt)
{
	if (!_Asset) return;

	using EStatus = CBehaviourTreeNodeBase::EStatus;

	U16 PrevIdx = 0;
	U16 NextIdx = 0;
	U16 CurrIdx = 0;
	U16 NewLevel = 0;
	EStatus Status = EStatus::Running;

	_pNewStack[0] = 0;

	while (true)
	{
		const auto* pCurrNode = _Asset->GetNode(CurrIdx);

		// If traversing down and following the current path
		if (CurrIdx >= PrevIdx && NewLevel < _CurrDepth && CurrIdx == _pCurrStack[NewLevel])
		{
			// Update an already active node
			std::tie(Status, NextIdx) = pCurrNode->pNodeImpl->Update();

			// The most often case for the node is to request itself when explicit traversal change is not needed
			if (NextIdx == CurrIdx)
			{
				// If no children, return to parent. Else proceed to the next current node or overriding high priority request.
				const auto NextLevel = NewLevel + 1;
				NextIdx = (NextLevel == _CurrDepth) ? pCurrNode->SkipSubtreeIndex : _pRequestStack[NextLevel];
			}
		}
		else
		{
			// Try to find a new active path in the tree
			NextIdx = pCurrNode->pNodeImpl->Traverse(PrevIdx, NextIdx, Status);

			// If the node requests itself, it is the new active node
			if (NextIdx == CurrIdx)
			{
				// When we have no active tree we will activate nodes starting from the root
				U16 CurrLevel = 0;

				// Deactivate the previously active subtree up to common parent with the new one.
				// NB: both stacks always start from the root so [0] is always identical and CurrLevel won't become negative.
				if (_CurrDepth)
				{
					CurrLevel = _CurrDepth - 1;
					while (CurrLevel > NewLevel || _pCurrStack[CurrLevel] != _pNewStack[CurrLevel])
					{
						_Asset->GetNode(_pCurrStack[CurrLevel])->pNodeImpl->Deactivate();
						--CurrLevel;
					}

					// Proceed from the common parent to the first child on the new path (if any)
					++CurrLevel;
				}

				// Activate the new subtree
				while (CurrLevel <= NewLevel)
				{
					const auto ActivatingIdx = _pNewStack[CurrLevel];
					Status = _Asset->GetNode(ActivatingIdx)->pNodeImpl->Activate();

					if (Status == EStatus::Failed)
					{
						// Rollback to the last successfully activated node and stop activation
						NewLevel = CurrLevel - 1;
						pCurrNode = _Asset->GetNode(_pCurrStack[ActivatingIdx - 1]);
						break;
					}

					_pCurrStack[CurrLevel++] = ActivatingIdx;
				}

				// CurrLevel is now 1 level past the last activated node
				_CurrDepth = CurrLevel;

				// A new subtree was activated and the status must be returned to the active node's parent regardless of the value.
				// NB: an active node can't currently proceed down the tree, it sould have done that in Traverse. May change later.
				NextIdx = pCurrNode->SkipSubtreeIndex;
			}
		}

		if (NextIdx >= pCurrNode->SkipSubtreeIndex)
		{
			// Subtree is finished, must return to the parent
			if (!NewLevel) break;
			CurrIdx = _pNewStack[--NewLevel];
		}
		else
		{
			_pNewStack[++NewLevel] = NextIdx;
			PrevIdx = CurrIdx;
			CurrIdx = NextIdx;
		}
	}

	// Initially we have no higher priority requests and default to continuing along the current active path
	std::copy_n(_pCurrStack, _CurrDepth, _pRequestStack);

	//!!!must check that after failed activation and rollback the newly activated node won't update on the next iteration!

	// if Status = failed here does it always mean that non-empty current stack was failed to activate and must be aborted to the root?!

	// at the end Status contains the root status, can detect tree execution end to stop or loop!
}
//---------------------------------------------------------------------

}
