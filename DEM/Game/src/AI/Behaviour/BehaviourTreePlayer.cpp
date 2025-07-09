#include "BehaviourTreePlayer.h"
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <AI/AIStateComponent.h>
#include <Game/ECS/Components/ActionQueueComponent.h>
#include <Game/ECS/GameWorld.h>
#include <Events/Connection.h> // for destruction
#include <Math/Math.h>

namespace DEM::AI
{
constexpr auto BT_PLAYER_MIN_BUFFER_ALIGNMENT = sizeof(void*);
static_assert(__STDCPP_DEFAULT_NEW_ALIGNMENT__ >= BT_PLAYER_MIN_BUFFER_ALIGNMENT);

template<typename T>
static inline size_t CalcStackSize(const CBehaviourTreeAsset& Asset)
{
	return Math::CeilToMultiple(Asset.GetMaxDepth() * sizeof(T), BT_PLAYER_MIN_BUFFER_ALIGNMENT);
}
//---------------------------------------------------------------------

static inline size_t CalcBufferSize(const CBehaviourTreeAsset& Asset)
{
	const auto TraversalStackBytes = CalcStackSize<U16>(Asset);
	const auto InstanceDataPtrStackBytes = CalcStackSize<CBehaviourTreePlayer::CDataStackRecord>(Asset);
	return 3 * TraversalStackBytes + InstanceDataPtrStackBytes + Asset.GetMaxInstanceBytes();
}
//---------------------------------------------------------------------

CBehaviourTreePlayer::CBehaviourTreePlayer() = default;
CBehaviourTreePlayer::CBehaviourTreePlayer(CBehaviourTreePlayer&&) noexcept = default;
CBehaviourTreePlayer& CBehaviourTreePlayer::operator =(CBehaviourTreePlayer&& Other) noexcept = default;
//---------------------------------------------------------------------

CBehaviourTreePlayer::~CBehaviourTreePlayer()
{
	Stop();
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::SetAsset(PBehaviourTreeAsset Asset)
{
	const auto PrevBytes = _Asset ? CalcBufferSize(*_Asset) : 0;

	Stop();

	_Asset = std::move(Asset);
	if (!_Asset) return;

	// Allocate a single buffer for traversal stacks and node instance data
	const auto NewBytes = CalcBufferSize(*_Asset);
	if (PrevBytes < NewBytes)
	{
		_MemBuffer.reset(new std::byte[NewBytes]);
		n_assert_dbg(Math::IsAligned<BT_PLAYER_MIN_BUFFER_ALIGNMENT>(_MemBuffer.get()));
	}

	static_assert(std::is_trivial_v<U16>, "U16 must be a trivial type to skip explicit construction!"); // just in case
	static_assert(std::is_trivial_v<CDataStackRecord>, "CDataStackRecord must be a trivial type to skip explicit construction!");

	const auto TraversalStackBytes = CalcStackSize<U16>(*_Asset);
	const auto InstanceDataPtrStackBytes = CalcStackSize<CDataStackRecord>(*_Asset);

	auto* pRawMem = _MemBuffer.get();

	_pNewStack = reinterpret_cast<U16*>(pRawMem);
	pRawMem += TraversalStackBytes;

	_pRequestStack = reinterpret_cast<U16*>(pRawMem);
	pRawMem += TraversalStackBytes;

	_pActiveStack = reinterpret_cast<U16*>(pRawMem);
	pRawMem += TraversalStackBytes;

	_pNodeInstanceData = reinterpret_cast<CDataStackRecord*>(pRawMem);
	pRawMem += InstanceDataPtrStackBytes;

	_pInstanceDataBuffer = pRawMem;
}
//---------------------------------------------------------------------

bool CBehaviourTreePlayer::Start(Game::CGameSession& Session, Game::HEntity ActorID)
{
	if (!_Asset || !ActorID) return false;

	_pSession = &Session;
	_ActorID = ActorID;

	for (U16 i = 0; i < _Asset->GetNodeCount(); ++i)
		_Asset->GetNode(i)->pNodeImpl->OnTreeStarted(i, *this);

	// All blackboard changes are handled by a single subscription. It is both optimal
	// and deals with the lack of knowledge about a blackboard in a condition system.
	if (!_BBKeyToNode.empty())
	{
		auto* pWorld = _pSession->FindFeature<Game::CGameWorld>();
		auto* pBrain = pWorld->FindComponent<const CAIStateComponent>(_ActorID);
		_NodeSubs.push_back(pBrain->Blackboard.OnChanged.Subscribe([this](HVar BBKey)
		{
			// Empty BBKey means the whole blackboard change
			const auto Range = BBKey ? _BBKeyToNode.equal_range(BBKey) : std::make_pair(_BBKeyToNode.begin(), _BBKeyToNode.end());
			for (auto It = Range.first; It != Range.second; ++It)
				RequestEvaluation(It->second);
		}));
	}

	return true;
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::Stop()
{
	// Is not playing without a session
	if (!_pSession) return;

	// Deactivate an active subtree even if there are missing components
	auto* pWorld = _pSession->FindFeature<Game::CGameWorld>();
	auto* pBrain = pWorld ? pWorld->FindComponent<CAIStateComponent>(_ActorID) : nullptr;
	auto* pActuator = pWorld ? pWorld->FindComponent<Game::CActionQueueComponent>(_ActorID) : nullptr;
	ResetActivePath(CBehaviourTreeContext{ *_pSession, _ActorID, pBrain, pActuator });

	_BBKeyToNode.clear();
	_NodeSubs.clear();
	_pSession = nullptr;
	_ActorID = {};
}
//---------------------------------------------------------------------

//???instead rewrite to Push/PopActiveNode? Then level will come from _ActiveDepth!
EBTStatus CBehaviourTreePlayer::ActivateNode(U16 Index, CDataStackRecord& InstanceDataRecord, const CBehaviourTreeContext& Ctx)
{
	auto* pNode = _Asset->GetNode(Index);
	n_assert_dbg(pNode);
	if (!pNode) return EBTStatus::Failed;

	auto* pNodeImpl = pNode->pNodeImpl;

	// Allocate bytes for node instance data
	InstanceDataRecord.pPrevStackTop = _pInstanceDataBuffer;
	if (const auto DataSize = pNodeImpl->GetInstanceDataSize())
	{
		//???FIXME: must also consider MinInstanceAlignment from asset loading? or now it is guaranteed to work correctly?
		InstanceDataRecord.pNodeData = Math::NextAligned(_pInstanceDataBuffer, pNodeImpl->GetInstanceDataAlignment());
		_pInstanceDataBuffer = InstanceDataRecord.pNodeData + DataSize;
	}
	else
	{
		InstanceDataRecord.pNodeData = nullptr;
	}

	const auto Status = pNodeImpl->Activate(InstanceDataRecord.pNodeData, Ctx);

	// On failure cancel effects of possible partial activation and free allocated bytes
	if (Status == EBTStatus::Failed)
		DeactivateNode(Index, InstanceDataRecord, Ctx);

	return Status;
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::DeactivateNode(U16 Index, CDataStackRecord& InstanceDataRecord, const CBehaviourTreeContext& Ctx)
{
	_Asset->GetNode(Index)->pNodeImpl->Deactivate(InstanceDataRecord.pNodeData, Ctx);

	// Free bytes allocated for this instance
	_pInstanceDataBuffer = InstanceDataRecord.pPrevStackTop;
}
//---------------------------------------------------------------------

void CBehaviourTreePlayer::ResetActivePath(const CBehaviourTreeContext& Ctx)
{
	while (_ActiveDepth)
	{
		--_ActiveDepth;
		DeactivateNode(_pActiveStack[_ActiveDepth], _pNodeInstanceData[_ActiveDepth], Ctx);
	}
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreePlayer::Update(float dt)
{
	if (!_Asset || !_pSession || !_ActorID) return EBTStatus::Failed;

	auto* pWorld = _pSession->FindFeature<Game::CGameWorld>();
	if (!pWorld) return EBTStatus::Failed;

	auto* pBrain = pWorld->FindComponent<CAIStateComponent>(_ActorID);
	if (!pBrain) return EBTStatus::Failed;

	auto* pActuator = pWorld->FindComponent<Game::CActionQueueComponent>(_ActorID);
	if (!pActuator) return EBTStatus::Failed;

	// Proceed to update only when we have all necessary components
	CBehaviourTreeContext Ctx{ *_pSession, _ActorID, pBrain, pActuator };

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
			std::tie(Status, NextIdx) = pNode->pNodeImpl->Update(CurrIdx, _pNodeInstanceData[NewLevel].pNodeData, dt, Ctx);

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
				std::tie(Status, NextIdx) = pNode->pNodeImpl->TraverseFromParent(CurrIdx, pNode->SkipSubtreeIndex, Ctx);
			else
				std::tie(Status, NextIdx) = pNode->pNodeImpl->TraverseFromChild(CurrIdx, pNode->SkipSubtreeIndex, NextIdx, Status, Ctx);

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
						DeactivateNode(_pActiveStack[Level], _pNodeInstanceData[Level], Ctx);
						--Level;
					}

					// Proceed from the common parent to the first child on the new path (if any)
					++Level;
				}

				// Activate the new subtree
				while (Level <= NewLevel)
				{
					const auto ActivatingIdx = _pNewStack[Level];
					Status = ActivateNode(ActivatingIdx, _pNodeInstanceData[Level], Ctx);

					n_assert_dbg(static_cast<size_t>(std::distance(_pNodeInstanceData[0].pPrevStackTop, _pInstanceDataBuffer)) < _Asset->GetMaxInstanceBytes());

					// NB: difference between Succeeded and Running is important for the deepest active action as it is passed to parents
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
				// NB: the deepest active node can't currently proceed down the tree, it should have done that in Traverse. May change later.
				NextIdx = pNode->SkipSubtreeIndex;
			}
		}

		// Make sure that we either descend to one of children or return to the parent and not jump randomly
		n_assert_dbg(NextIdx > CurrIdx && NextIdx <= pNode->SkipSubtreeIndex);

		// Make sure that we don't return to one of previous children of the same parent, literally:
		// we either descended here (not returned from child at all) or that child goes before the one we are going to
		n_assert_dbg(IsGoingDown || PrevIdx < NextIdx);

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
	if (Status != EBTStatus::Running) ResetActivePath(Ctx);

	return Status;
}
//---------------------------------------------------------------------

bool CBehaviourTreePlayer::RequestEvaluation(U16 Index)
{
	// Nothing to override, evaluation will start from the root
	if (!_ActiveDepth) return false;

	// Only higher priority nodes can override the active path
	const U16 ActiveLevel = _ActiveDepth - 1;
	if (_pActiveStack[ActiveLevel] <= Index) return false;

	// Bubble request up to the common parent on the active path (the root in the worst case)
	U16 CandidateIdx = Index;
	U16 CurrIdx = Index;
	const auto* pNode = _Asset->GetNode(CurrIdx);
	while (ActiveLevel > pNode->DepthLevel || CurrIdx < _pActiveStack[pNode->DepthLevel])
	{
		if (!pNode->pNodeImpl->CanOverrideLowerPriorityNodes()) return false;

		CandidateIdx = CurrIdx;
		CurrIdx = pNode->ParentIndex;
		pNode = _Asset->GetNode(CurrIdx);
	}

	// CandidateIdx is the first divergence point between the request path and the active path
	const auto CandidateLevel = _Asset->GetNode(CandidateIdx)->DepthLevel;
	auto& RequestSlot = _pRequestStack[CandidateLevel];

	// A sibling with higher priority already requested evaluation
	if (CandidateIdx >= RequestSlot) return false;

	// Finally register a request
	RequestSlot = CandidateIdx;
	return true;
}
//---------------------------------------------------------------------

// FIXME: BB can be obtained from _pSession + _ActorID. Passing BB here is an optimization, but should really do it this way?
void CBehaviourTreePlayer::EvaluateOnBlackboardChange(const CBlackboard& BB, CStrID BBKey, U16 Index)
{
	//!!!FIXME: what if the key is not used yet but the value will be assigned later?! Must pre-register all keys in a blackboard? Need to know types in that place!
	//May use storage's Load() to initialize  the blackboard from HRD!
	if (auto Handle = BB.GetStorage().Find(BBKey))
		_BBKeyToNode.emplace(Handle, Index);
}
//---------------------------------------------------------------------

}
