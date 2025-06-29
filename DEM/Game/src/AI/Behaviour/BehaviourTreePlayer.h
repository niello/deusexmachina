#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <map>

// Plays a CBehaviourTreeAsset and tracks its state. Parallel tasks can be implemented using their own nested players.

namespace DEM::Events
{
	class CConnection;
}

struct HVar;

namespace DEM::AI
{
using PBehaviourTreeAsset = Ptr<class CBehaviourTreeAsset>;
enum class EBTStatus : U8;
struct CBehaviourTreeContext;

class CBehaviourTreePlayer final
{
public:

	struct CDataStackRecord
	{
		std::byte* pNodeData;
		std::byte* pPrevStackTop;
	};

private:

	PBehaviourTreeAsset               _Asset;
	std::vector<Events::CConnection>& _NodeSubs;
	std::multimap<HVar, U16>          _BBKeyToNode; // A map of BB keys to nodes that are affected by its change

	std::unique_ptr<std::byte[]>      _MemBuffer;
	U16*                              _pNewStack = nullptr;           // Current traversal path
	U16*                              _pRequestStack = nullptr;       // Currently activated path overridden by requests from higher priority nodes
	U16*                              _pActiveStack = nullptr;        // Currently activated path
	CDataStackRecord*                 _pNodeInstanceData = nullptr;   // Pointers to node instance data of active path nodes
	std::byte*                        _pInstanceDataBuffer = nullptr; // Top of the stack allocation buffer for node instance data

	U16                               _ActiveDepth = 0;               // Depth of _pActiveStack

	EBTStatus ActivateNode(U16 Index, CDataStackRecord& InstanceDataRecord, const CBehaviourTreeContext& Ctx);
	void      DeactivateNode(U16 Index, CDataStackRecord& InstanceDataRecord, const CBehaviourTreeContext& Ctx);
	void      ResetActivePath(const CBehaviourTreeContext& Ctx);

public:

	~CBehaviourTreePlayer();

	void      SetAsset(PBehaviourTreeAsset Asset);
	bool      Start(const CBehaviourTreeContext& Ctx);
	void      Stop();
	EBTStatus Update(const CBehaviourTreeContext& Ctx, float dt);
	bool      RequestEvaluation(U16 Index);
	void      EvaluateOnBlackboardChange(const CBehaviourTreeContext& Ctx, CStrID BBKey, U16 Index);

	CBehaviourTreeAsset* GetAsset() const { return _Asset.Get(); }
	auto&                Subscriptions() { return _NodeSubs; }
	bool                 IsPlaying() const { return _ActiveDepth > 0; } //???or !!_Asset? or explicit flag?!
};

}
