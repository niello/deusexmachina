#pragma once
#include <Animation/Graph/SelectorNodeBase.h>
#include <Data/StringID.h>

// Animation graph node that selects a subnode based on an integer value being in range [From; To]

namespace DEM::Anim
{
using PIntSelectorNode = std::unique_ptr<class CIntSelectorNode>;

class CIntSelectorNode : public CSelectorNodeBase
{
protected:

	struct CRecord
	{
		CVariant Variant;
		int      From;
		int      To;
	};

	CStrID               _ParamID;
	UPTR                 _ParamIndex = INVALID_INDEX; // Cached for fast access

	std::vector<CRecord> _Variants;

	virtual CVariant* SelectVariant(CAnimationUpdateContext& Context) override;

public:

	CIntSelectorNode(CStrID ParamID);

	void AddVariant(PAnimGraphNode&& Node, int From, int To, float BlendTime = 0.f, U32 InterruptionPriority = 0);

	virtual void Init(CAnimationInitContext& Context) override;
};

}
