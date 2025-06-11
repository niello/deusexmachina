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
	return TraversalStackBytes + TraversalStackBytes + Asset.GetMaxInstanceBytes();
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
		_pCurrStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get()));
		_pPrevStack = std::launder(reinterpret_cast<U16*>(_MemBuffer.get() + TraversalStackBytes));
		_pNodeInstanceData = _MemBuffer.get() + TraversalStackBytes + TraversalStackBytes;
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
	// if search requested or there is no active subtree, do search
	//_RequestedTraversalStartIndex

	// update active subtree; on node activation allocate a buffer

	// search and update interchange!
}
//---------------------------------------------------------------------

}
