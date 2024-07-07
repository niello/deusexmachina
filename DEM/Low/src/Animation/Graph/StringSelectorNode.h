#pragma once
#include <Animation/Graph/SelectorNodeBase.h>
#include <Data/StringID.h>
#include <Data/VarStorage.h> // for HVar

// Animation graph node that selects a subnode based on a string value

namespace DEM::Anim
{
using PStringSelectorNode = std::unique_ptr<class CStringSelectorNode>;

class CStringSelectorNode : public CSelectorNodeBase
{
protected:

	struct CRecord
	{
		CVariant Variant;
		CStrID   Value;
	};

	CStrID               _ParamID;
	HVar                 _ParamHandle; // Cached for fast access

	std::vector<CRecord> _Variants;
	CVariant             _DefaultVariant;

	virtual CVariant* SelectVariant(CAnimationUpdateContext& Context) override;

public:

	CStringSelectorNode(CStrID ParamID);

	virtual void Init(CAnimationInitContext& Context) override;

	void AddVariant(PAnimGraphNode&& Node, CStrID Value, float BlendTime = 0.f, U32 InterruptionPriority = 0);
	void SetDefaultVariant(PAnimGraphNode&& Node, float BlendTime = 0.f, U32 InterruptionPriority = 0);
};

}
