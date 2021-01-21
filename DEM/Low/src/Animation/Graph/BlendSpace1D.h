#pragma once
#include <Animation/Graph/AnimGraphNode.h>
#include <Animation/AnimationBlender.h>
#include <Data/StringID.h>

// Animation graph node that blends sources based on a value in 1D space

namespace DEM::Anim
{
using PBlendSpace1D = std::unique_ptr<class CBlendSpace1D>;

//???or inherit from some base class for all blends? or just contain CAnimationBlender instead (if will use it here).
//May share phase sync code between all blend spaces! Or more generalized sync?
class CBlendSpace1D : public CAnimGraphNode
{
protected:

	static inline constexpr float SAMPLE_MATCH_TOLERANCE = 0.001f; //???or relative to parameter space?

	struct CSample
	{
		PAnimGraphNode Source;
		float          Value;
	};

	std::vector<CSample> _Samples; // Sorted by value ascending
	CAnimationBlender    _Blender;
	CAnimGraphNode*      _pFirst = nullptr;
	CAnimGraphNode*      _pSecond = nullptr;

	CStrID               _ParamID;
	UPTR                 _ParamIndex = INVALID_INDEX; // Cached for fast access

	float                _NormalizedTime = 0.f; // Current time normalized to [0..1]

	void          UpdateSingleSample(CAnimGraphNode& Node, CAnimationController& Controller, float dt, CSyncContext* pSyncContext);

public:

	CBlendSpace1D(CStrID ParamID);

	virtual void  Init(CAnimationControllerInitContext& Context) override;
	virtual void  Update(CAnimationController& Controller, float dt, CSyncContext* pSyncContext) override;
	virtual void  EvaluatePose(IPoseOutput& Output) override;

	virtual float GetAnimationLengthScaled() const override;
	virtual float GetLocomotionPhase() const override;

	bool          AddSample(PAnimGraphNode&& Source, float Value);
};

}
