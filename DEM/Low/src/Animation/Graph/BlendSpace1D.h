#pragma once
#include <Animation/Graph/AnimGraphNode.h>

// Base for all animation graph nodes

namespace DEM::Anim
{
//using PBlendSpace1D = std::unique_ptr<class CBlendSpace1D>;

class CBlendSpace1D : public CAnimGraphNode //???or some base class for all blends? or just contain CAnimationBlender instead.
{
protected:

	//!!!need tolerance! value near the sample must use only that sample, to avoid blending 0.99999 + 0.00001 etc!
	//also ensure all samples are different for more than tolerance!
	//???tolerance is constexpr? Can be such, if use relative value (normalized offset). Is good?

	// blend space params
	// samples => value + source (any CAnimGraphNode can be a source)

public:

	//virtual bool BindOutput() - Prepare/Tune external output instead of binding and storing into the node?
	virtual void Update(float dt/*, params*/) override;
	virtual void EvaluatePose(/*IPoseOutput& but only if not bound!*/) override;
};

}
