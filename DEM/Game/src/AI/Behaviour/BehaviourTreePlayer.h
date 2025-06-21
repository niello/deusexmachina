#pragma once
#include <Data/Ptr.h>

// Plays a CBehaviourTreeAsset and tracks its state. Parallel tasks can be implemented using their own nested players.

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::AI
{
using PBehaviourTreeAsset = Ptr<class CBehaviourTreeAsset>;

class CBehaviourTreePlayer final
{
private:

	PBehaviourTreeAsset          _Asset;
	std::unique_ptr<std::byte[]> _MemBuffer;
	U16*                         _pNewStack = nullptr;         // Current traversal path
	U16*                         _pActiveStack = nullptr;      // Currently activated path
	U16*                         _pRequestStack = nullptr;     // Currently activated path overridden by requests from higher priority nodes
	std::byte*                   _pNodeInstanceData = nullptr;
	U16                          _ActiveDepth = 0;             // Depth of _pActiveStack

public:

	~CBehaviourTreePlayer();

	bool Start(PBehaviourTreeAsset Asset);
	void Stop();
	void Update(Game::CGameSession& Session, float dt);

	CBehaviourTreeAsset* GetAsset() const { return _Asset.Get(); }
	//bool                 IsPlaying() const { return _CurrAction || _NextActionID != EmptyActionID; }
};

}
