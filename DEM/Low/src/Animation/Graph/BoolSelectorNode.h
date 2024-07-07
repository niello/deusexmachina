#pragma once
#include <Animation/Graph/SelectorNodeBase.h>
#include <Data/StringID.h>
#include <Data/VarStorage.h> // for HVar

// Animation graph node that selects a subnode based on a boolean value

namespace DEM::Anim
{
using PBoolSelectorNode = std::unique_ptr<class CBoolSelectorNode>;

class CBoolSelectorNode : public CSelectorNodeBase
{
protected:

	CStrID               _ParamID;
	HVar                 _ParamHandle; // Cached for fast access

	CVariant             _TrueVariant;
	CVariant             _FalseVariant;

	virtual CVariant* SelectVariant(CAnimationUpdateContext& Context) override;

public:

	CBoolSelectorNode(CStrID ParamID);

	virtual void Init(CAnimationInitContext& Context) override;

	void SetTrueNode(PAnimGraphNode&& Node, float BlendTime = 0.f, U32 InterruptionPriority = 0);
	void SetFalseNode(PAnimGraphNode&& Node, float BlendTime = 0.f, U32 InterruptionPriority = 0);
};

}
