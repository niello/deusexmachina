#include "BehaviourTreePlayer.h"
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <Math/Math.h>

namespace DEM::AI
{
constexpr auto BT_PLAYER_MIN_BUFFER_ALIGNMENT = sizeof(void*);

template<typename T>
static inline size_t CalcStackSize(const CBehaviourTreeAsset& Asset)
{
	return Math::CeilToMultiple(Asset.GetMaxDepth() * sizeof(T), BT_PLAYER_MIN_BUFFER_ALIGNMENT);
}
//---------------------------------------------------------------------

static inline size_t CalcBufferSize(const CBehaviourTreeAsset& Asset)
{
	const auto TraversalStackBytes = CalcStackSize<U16>(Asset);
	const auto InstanceDataPtrStackBytes = CalcStackSize<std::byte*>(Asset);
	return 3 * TraversalStackBytes + InstanceDataPtrStackBytes + Asset.GetMaxInstanceBytes();
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
		n_assert_dbg(Math::IsAligned<BT_PLAYER_MIN_BUFFER_ALIGNMENT>(_MemBuffer.get()));

		const auto TraversalStackBytes = CalcStackSize<U16>(*_Asset);
		const auto InstanceDataPtrStackBytes = CalcStackSize<std::byte*>(*_Asset);
		_pNewStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get()));
		_pRequestStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get() + TraversalStackBytes));
		_pActiveStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get() + 2 * TraversalStackBytes));
		_pNodeInstanceData = std::launder(reinterpret_cast<std::byte**>(_MemBuffer.get() + 3 * TraversalStackBytes));
		_pInstanceDataBuffer = _MemBuffer.get() + 3 * TraversalStackBytes + InstanceDataPtrStackBytes;
	}

	return true;
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::Stop()
{
	while (_ActiveDepth)
		DeactivateNode(_pActiveStack[--_ActiveDepth]);
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreePlayer::ActivateNode(U16 Index)
{
	auto* pNode = _Asset->GetNode(Index);
	n_assert_dbg(pNode);
	if (!pNode) return EBTStatus::Failed;

	auto* pNodeImpl = pNode->pNodeImpl;
	const auto DataSize = pNodeImpl->GetInstanceDataSize();
	const auto Alignment = pNodeImpl->GetInstanceDataAlignment();
	if (DataSize && Alignment)
	{
		_pNodeInstanceData = Math::NextAligned(_pNodeInstanceData, Alignment);
		_pNodeInstanceData += pNodeImpl->GetInstanceDataSize();

		// push real offset to stack
	}
	else
	{
		// push 0 offset to stack, could even push nullptr!
	}

	const auto Status = pNodeImpl->Activate();

	if (Status == EBTStatus::Failed)
	{
		// pop instance data
		// pop offset from stack
		// new end = offset + GetInstanceDataSize of prev node
		// would need to rewind O(n) for nodes without size
		//instead of pointer can store two offsets U32 as a pair or 24:8 as struct!
	}

	return Status;
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::DeactivateNode(U16 Index)
{
	_Asset->GetNode(Index)->pNodeImpl->Deactivate();

	// pop instance data
	// pop offset from stack
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreePlayer::Update(Game::CGameSession& Session, float dt)
{
	if (!_Asset) return EBTStatus::Failed;

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

		//!!!TODO: assert returning to the child <= one we returned from! potential infinite loop!

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
						DeactivateNode(_pActiveStack[Level--]);

					// Proceed from the common parent to the first child on the new path (if any)
					++Level;
				}

				// Activate the new subtree
				while (Level <= NewLevel)
				{
					const auto ActivatingIdx = _pNewStack[Level];
					Status = ActivateNode(ActivatingIdx);

					//???TODO: what if succeeded? now proceeds to the child, like for Running. Instead of status activation must return only bool?
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

	// If the tree is not running, the current active path is no longer active and must be deactivated
	if (Status != EBTStatus::Running) Stop();

	return Status;
}
//---------------------------------------------------------------------

}
