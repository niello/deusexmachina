#pragma once
#include <Data/Ptr.h>

// Plays a BehaviourTree asset and tracks its state. Parallel tasks use their sub-players.

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
	std::unique_ptr<std::byte[]> _NodeInstanceData;

public:

	~CBehaviourTreePlayer();

	bool Start(PBehaviourTreeAsset Asset);
	void Stop();
	void Update(Game::CGameSession& Session, float dt);

	CBehaviourTreeAsset* GetAsset() const { return _Asset.Get(); }
	//bool                 IsPlaying() const { return _CurrAction || _NextActionID != EmptyActionID; }
};

}
