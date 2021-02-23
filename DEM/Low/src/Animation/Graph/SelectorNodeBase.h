#pragma once
#include <Animation/Graph/AnimGraphNode.h>

// Animation graph node that selects a child node and blends into it

namespace DEM::Anim
{
using PSelectorNodeBase = std::unique_ptr<class CSelectorNodeBase>;

class CSelectorNodeBase : public CAnimGraphNode
{
protected:

	struct CVariant
	{
		PAnimGraphNode Node;
		float          BlendTime = 0.f;
		U32            InterruptionPriority = 0;
	};

	CVariant* _pCurrVariant = nullptr;

	virtual CVariant* SelectVariant() = 0;

public:

	virtual void Init(CAnimationInitContext& Context) override;
	virtual void Update(CAnimationUpdateContext& Context, float dt) override;
	virtual void EvaluatePose(CPoseBuffer& Output) override;

	virtual float GetAnimationLengthScaled() const override;
};

}
