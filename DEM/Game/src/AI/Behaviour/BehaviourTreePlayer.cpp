#include "BehaviourTreePlayer.h"
#include <AI/Behaviour/BehaviourTreeAsset.h>

namespace DEM::AI
{

CBehaviourTreePlayer::~CBehaviourTreePlayer()
{
	Stop();
}
//---------------------------------------------------------------------

bool CBehaviourTreePlayer::Start(PBehaviourTreeAsset Asset)
{
	const auto PrevDepth = _Asset ? _Asset->GetMaxDepth() : 0;
	const auto PrevInstanceBytes = _Asset ? _Asset->GetMaxInstanceBytes() : 0;

	Stop();

	_Asset = std::move(Asset);
	if (!_Asset) return false;

	if (PrevDepth < _Asset->GetMaxDepth())
	{
		// alloc two stacks, each of max depth, will swap them as prev/new for subtree switching
		// will store indices, but of which type? is U16 enough for any BT?
	}

	if (PrevInstanceBytes < _Asset->GetMaxInstanceBytes())
	{
		_NodeInstanceData.reset(new std::byte[_Asset->GetMaxInstanceBytes()]);
		n_assert_dbg(IsAligned<sizeof(void*)>(_NodeInstanceData.get()));
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
	// if search requested, do search

	// update active subtree
}
//---------------------------------------------------------------------

}
