#pragma once
#include <Data/Params.h>
#include <Data/Metadata.h>
#include <Core/Object.h>

// Reusable behaviour tree asset

namespace DEM::AI
{

struct CBehaviourTreeNodeData
{
	CStrID                              ClassName; //TODO: use FourCC?! or register class names in a factory as CStrIDs?!
	Data::PParams                       Params;
	std::vector<CBehaviourTreeNodeData> Children;
};

class CBehaviourTreeNodeBase : public Core::CRTTIBaseClass
{
public:

	enum class EStatus : U8
	{
		Running,
		Succeeded,
		Failed
	};

	virtual void                    Init(const Data::CParams* pParams) = 0;
	virtual size_t                  GetInstanceDataSize() const = 0;
	virtual size_t                  GetInstanceDataAlignment() const = 0;

	virtual U16                     Traverse(U16 PrevIdx, U16 NextIdx, EStatus ChildStatus) const = 0;
	virtual EStatus                 Activate() const = 0;
	virtual void                    Deactivate() const = 0;
	virtual std::pair<EStatus, U16> Update() const = 0;
};

class CBehaviourTreeAsset : public DEM::Core::CObject
{
	RTTI_CLASS_DECL(DEM::AI::CBehaviourTreeAsset, DEM::Core::CObject);

public:

	struct CNode
	{
		CBehaviourTreeNodeBase* pNodeImpl;
		U16                     SkipSubtreeIndex;
	};

protected:

	std::unique_ptr<CNode[]> _Nodes;
	unique_ptr_aligned<void> _NodeImplBuffer;

	size_t                   _MaxDepth = 0;
	size_t                   _MaxInstanceBytes = 0;

public:

	CBehaviourTreeAsset(CBehaviourTreeNodeData&& RootNodeData);
	~CBehaviourTreeAsset();

	size_t       GetNodeCount() const { return _Nodes ? _Nodes[0].SkipSubtreeIndex : 0; }
	const CNode* GetNode(U16 i) const { return &_Nodes[i]; }
	size_t       GetMaxDepth() const { return _MaxDepth; }
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
