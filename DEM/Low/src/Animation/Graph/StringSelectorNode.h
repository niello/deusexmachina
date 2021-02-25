#pragma once
#include <Animation/Graph/SelectorNodeBase.h>
#include <Data/StringID.h>

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
	UPTR                 _ParamIndex = INVALID_INDEX; // Cached for fast access

	std::vector<CRecord> _Variants;

	virtual CVariant* SelectVariant(CAnimationUpdateContext& Context) override;

public:

	CStringSelectorNode(CStrID ParamID);

	virtual void Init(CAnimationInitContext& Context) override;

	void AddVariant(PAnimGraphNode&& Node, CStrID Value, float BlendTime = 0.f, U32 InterruptionPriority = 0);
};

}
