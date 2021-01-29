#pragma once
#include <Animation/Graph/AnimGraphNode.h>
#include <Animation/AnimationBlender.h>
#include <Data/StringID.h>

// Animation graph node that blends sources based on two values in 2D space

namespace DEM::Anim
{
using PBlendSpace2D = std::unique_ptr<class CBlendSpace2D>;

class CBlendSpace2D : public CAnimGraphNode
{
public:

	struct CSample
	{
		PAnimGraphNode Source;
		float          XValue;
		float          YValue;
	};

protected:

	static inline constexpr float SAMPLE_MATCH_TOLERANCE = 0.001f; //???or relative to parameter space?

	struct CTriangle
	{
		uint32_t Adjacent[3];    // Indices of adjacent triangles per edge
		float    InvDenominator; // Cache
		float    ax, ay;
		float    abx, aby;
		float    acx, acy;
	};

	std::vector<CSample>   _Samples;
	std::vector<CTriangle> _Triangles;
	CAnimationBlender      _Blender;
	CAnimGraphNode*        _pFirst = nullptr;
	CAnimGraphNode*        _pSecond = nullptr;
	CAnimGraphNode*        _pThird = nullptr;

	CStrID                 _XParamID;
	CStrID                 _YParamID;
	UPTR                   _XParamIndex = INVALID_INDEX; // Cached for fast access
	UPTR                   _YParamIndex = INVALID_INDEX; // Cached for fast access

	float                  _NormalizedTime = 0.f; // Current time normalized to [0..1]

	void          UpdateSingleSample(CAnimGraphNode& Node, CAnimationController& Controller, float dt, CSyncContext* pSyncContext);

public:

	CBlendSpace2D(CStrID XParamID, CStrID YParamID);

	virtual void  Init(CAnimationControllerInitContext& Context) override;
	virtual void  Update(CAnimationController& Controller, float dt, CSyncContext* pSyncContext) override;
	virtual void  EvaluatePose(IPoseOutput& Output) override;

	virtual float GetAnimationLengthScaled() const override;
	virtual float GetLocomotionPhase() const override;

	bool          AddSample(PAnimGraphNode&& Source, float XValue, float YValue);
};

}
