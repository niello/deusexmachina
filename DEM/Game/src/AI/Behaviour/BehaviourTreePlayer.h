#pragma once
#include <Game/ECS/Entity.h>
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <map>

// Plays a CBehaviourTreeAsset and tracks its state. Parallel tasks can be implemented using their own nested players.

namespace DEM::Events
{
	class CConnection;
}

namespace DEM::Game
{
	class CGameSession;
}

struct HVar;

namespace DEM::AI
{
using PBehaviourTreeAsset = Ptr<class CBehaviourTreeAsset>;
enum class EBTStatus : U8;
struct CBehaviourTreeContext;
class CBlackboard;

class CBehaviourTreePlayer final
{
public:

	struct CDataStackRecord
	{
		std::byte* pNodeData;
		std::byte* pPrevStackTop;
	};

private:

	PBehaviourTreeAsset              _Asset;
	Game::CGameSession*              _pSession = nullptr;
	Game::HEntity                    _ActorID;
	std::vector<Events::CConnection> _NodeSubs;
	std::multimap<HVar, U16>         _BBKeyToNode; // A map of BB keys to nodes that are affected by its change

	std::unique_ptr<std::byte[]>     _MemBuffer;
	U16*                             _pNewStack = nullptr;           // Current traversal path
	U16*                             _pRequestStack = nullptr;       // Currently activated path overridden by requests from higher priority nodes
	U16*                             _pActiveStack = nullptr;        // Currently activated path
	CDataStackRecord*                _pNodeInstanceData = nullptr;   // Pointers to node instance data of active path nodes
	std::byte*                       _pInstanceDataBuffer = nullptr; // Top of the stack allocation buffer for node instance data

	U16                               _ActiveDepth = 0;               // Depth of _pActiveStack

	EBTStatus ActivateNode(U16 Index, CDataStackRecord& InstanceDataRecord, const CBehaviourTreeContext& Ctx);
	void      DeactivateNode(U16 Index, CDataStackRecord& InstanceDataRecord, const CBehaviourTreeContext& Ctx);
	void      ResetActivePath(const CBehaviourTreeContext& Ctx);

public:

	CBehaviourTreePlayer();
	CBehaviourTreePlayer(CBehaviourTreePlayer&&) noexcept;
	~CBehaviourTreePlayer();

	CBehaviourTreePlayer& operator =(CBehaviourTreePlayer&& Other) noexcept;

	void      SetAsset(PBehaviourTreeAsset Asset);
	bool      Start(Game::CGameSession& Session, Game::HEntity ActorID);
	void      Stop();
	EBTStatus Update(float dt);
	bool      RequestEvaluation(U16 Index);
	void      EvaluateOnBlackboardChange(const CBlackboard& BB, CStrID BBKey, U16 Index);

	CBehaviourTreeAsset* GetAsset() const { return _Asset.Get(); }
	Game::CGameSession*  GetSession() const { return _pSession; }
	Game::HEntity        GetActorID() const { return _ActorID; }
	auto&                Subscriptions() { return _NodeSubs; }
	bool                 IsPlaying() const { return !!_pSession; }
};

}
