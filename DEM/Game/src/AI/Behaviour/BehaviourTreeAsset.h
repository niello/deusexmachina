#pragma once
#include <Game/ECS/Entity.h>
#include <Data/Params.h>
#include <Data/Metadata.h>
#include <Core/Object.h>

// Reusable behaviour tree asset

namespace DEM::Game
{
	class CGameSession;
}

namespace DEM::AI
{
struct CAIStateComponent;
class CBehaviourTreePlayer;

struct CBehaviourTreeNodeData
{
	CStrID                              ClassName; //TODO: use FourCC?! or register class names in a factory as CStrIDs?!
	Data::PParams                       Params;
	std::vector<CBehaviourTreeNodeData> Children;
};

enum class EBTStatus : U8
{
	Running,
	Succeeded,
	Failed
};

struct CBehaviourTreeContext
{
	Game::CGameSession& Session;
	Game::HEntity       ActorID;
	CAIStateComponent*  pBrain;
};

class CBehaviourTreeNodeBase : public Core::CRTTIBaseClass
{
public:

	virtual void                      Init(const Data::CParams* pParams) {}
	virtual size_t                    GetInstanceDataSize() const { return 0; }
	virtual size_t                    GetInstanceDataAlignment() const { return 0; }

	virtual void                      OnTreeStarted(U16 SelfIdx, CBehaviourTreePlayer& Player, const CBehaviourTreeContext& Ctx) const {}
	virtual bool                      CanOverrideLowerPriorityNodes() const { return true; }

	virtual std::pair<EBTStatus, U16> TraverseFromParent(U16 SelfIdx, U16 SkipIdx, const CBehaviourTreeContext& Ctx) const { return { EBTStatus::Running, SelfIdx }; }
	virtual std::pair<EBTStatus, U16> TraverseFromChild(U16 SelfIdx, U16 SkipIdx, U16 NextIdx, EBTStatus ChildStatus, const CBehaviourTreeContext& Ctx) const { return { ChildStatus, NextIdx }; }
	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const { return EBTStatus::Running; }
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const {}
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const { return { EBTStatus::Running, SelfIdx }; }
};

class CBehaviourTreeAsset : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::AI::CBehaviourTreeAsset, DEM::Core::CObject);

public:

	struct CNode
	{
		CBehaviourTreeNodeBase* pNodeImpl;
		U16                     SkipSubtreeIndex;
		U16                     ParentIndex;
		U16                     DepthLevel; // 0 for the root
	};

protected:

	std::unique_ptr<CNode[]> _Nodes;
	unique_ptr_aligned<void> _NodeImplBuffer;

	U16                      _MaxDepth = 0;
	size_t                   _MaxInstanceBytes = 0;

public:

	CBehaviourTreeAsset(CBehaviourTreeNodeData&& RootNodeData);
	~CBehaviourTreeAsset();

	U16          GetNodeCount() const { return _Nodes ? _Nodes[0].SkipSubtreeIndex : 0; }
	const CNode* GetNode(U16 i) const { return &_Nodes[i]; }
	U16          GetMaxDepth() const { return _MaxDepth; }
	size_t       GetMaxInstanceBytes() const { return _MaxInstanceBytes; }
};

using PBehaviourTreeAsset = Ptr<CBehaviourTreeAsset>;

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CBehaviourTreeNodeData>() { return "DEM::AI::CBehaviourTreeNodeData"; }
template<> constexpr auto RegisterMembers<AI::CBehaviourTreeNodeData>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(AI::CBehaviourTreeNodeData, ClassName),
		DEM_META_MEMBER_FIELD(AI::CBehaviourTreeNodeData, Params),
		DEM_META_MEMBER_FIELD(AI::CBehaviourTreeNodeData, Children)
	);
}

}
