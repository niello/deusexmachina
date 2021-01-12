#pragma once
#include <Animation/Graph/AnimGraphNode.h>

// Animation graph node that blends sources based on a value in 1D space

namespace DEM::Anim
{
//using PBlendSpace1D = std::unique_ptr<class CBlendSpace1D>;

//???or inherit from some base class for all blends? or just contain CAnimationBlender instead (if will use it here).
//May share phase sync code between all blend spaces! Or more generalized sync?
class CBlendSpace1D : public CAnimGraphNode
{
protected:

	//!!!need tolerance! value near the sample must use only that sample, to avoid blending 0.99999 + 0.00001 etc!
	//also ensure all samples are different for more than tolerance!
	//???tolerance is constexpr? Can be such, if use relative value (normalized offset). Is good?

	// blend space params
	// samples => value + source (any CAnimGraphNode can be a source)

	//!!!blend space player must track curr time to blend all samples with this time! All samples are synchronized.
	//!!!NB: sync may be abs time, normalized time (0 - 1) or by phase matching (calc from feet or manual)!
	//???!!!Phase matching with monotone value may be much better than named markers?

	//!!!all sources must be pre-maped to output, but blender will always blend no more than two (three for 2D)!
	//???how to properly bind output without allocating excessive data (output per sample)?
	//???maybe blending on the fly is better here? Anyway no priority or additive, and weight always sum up to 1.

public:

	virtual void Init(CAnimationControllerInitContext& Context) override;
	//virtual bool BindOutput() - Prepare/Tune external output instead of binding and storing into the node?
	virtual void Update(float dt/*, params*/) override;
	virtual void EvaluatePose(IPoseOutput& Output) override;
};

}
