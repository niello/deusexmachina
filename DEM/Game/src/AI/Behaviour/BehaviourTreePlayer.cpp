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

	//std::fill_n(_pNewStack, _Asset->GetMaxDepth(), INVALID_INDEX_T<U16>);

	U16 CurrIdx = 0;
	_pNewStack[0] = 0;
	U16 CurrLevel = 0;
	EStatus Status = EStatus::Running;
	U16 PrevIdx = 0;
	U16 NextIdx = 0;

	while (true)
	{
		_pNewStack[CurrLevel] = CurrIdx;

		const auto* pCurrNode = _Asset->GetNode(CurrIdx);

		// If traversing down and following the current path
		if (CurrIdx >= PrevIdx && CurrLevel < _CurrDepth && CurrIdx == _pCurrStack[CurrLevel])
		{
			// Update an already active node
			std::tie(Status, NextIdx) = pCurrNode->pNodeImpl->Update();

			// The most often case for the node is to request itself when explicit traversal change is not needed
			if (NextIdx == CurrIdx)
			{
				// If no children, return to parent. Else choose between the next current node and overriding high priority request.
				const auto NextLevel = CurrLevel + 1;
				NextIdx = (NextLevel == _CurrDepth) ? pCurrNode->SkipSubtreeIndex : std::min(_pCurrStack[NextLevel], _pRequestStack[NextLevel]);
			}
		}
		else
		{
			// Try to find a new active path in the tree
			NextIdx = pCurrNode->pNodeImpl->Traverse(PrevIdx, NextIdx, Status);

			// If the node requests itself, it is the new active node
			if (NextIdx == CurrIdx)
			{
				// Deactivate the previously active subtree up to common parent with the new one
				auto ActiveLevel = _CurrDepth - 1;
				while (ActiveLevel > CurrLevel || _pCurrStack[ActiveLevel] != _pNewStack[ActiveLevel])
				{
					const auto& pActiveNode = _Asset->GetNode(_pCurrStack[ActiveLevel]);
					pActiveNode->pNodeImpl->Deactivate();
					--ActiveLevel;
				}

				// Activate the new subtree
				while (ActiveLevel < CurrLevel)
				{
					//???!!!must activate CurrLevel node but only if it isn't in the already active tree (rollback case)

					const auto ActivatingIdx = _pNewStack[ActiveLevel];
					const auto& pActivatingNode = _Asset->GetNode(ActivatingIdx);
					Status = pActivatingNode->pNodeImpl->Activate();

					if (Status == EStatus::Failed)
					{
						//???or deactivate the whole new subtree to common parent / the whole tree? if not reached target node
						CurrLevel = ActiveLevel;
						break;
					}
					else
					{
						_pCurrStack[ActiveLevel] = ActivatingIdx;
						++ActiveLevel;
					}
				}

				_CurrDepth = ActiveLevel + 1;

				// A new subtree was activated and now it must be left
				NextIdx = pCurrNode->SkipSubtreeIndex;
			}
		}

		if (NextIdx == pCurrNode->SkipSubtreeIndex)
		{
			// Subtree is finished, must return to the parent
			if (!CurrLevel) break;
			--CurrLevel;
		}
		else
		{
			++CurrLevel;
			PrevIdx = CurrIdx;
			CurrIdx = NextIdx;
		}
	}

	// at the end Status contains the root status, can detect tree execution end to stop or loop!
}
//---------------------------------------------------------------------

}
