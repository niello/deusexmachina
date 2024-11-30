#pragma once
#include <Animation/Graph/AnimGraphNode.h>
#include <Animation/PoseBuffer.h>
#include <Data/StringID.h>
#include <Data/VarStorage.h> // for HVar
#include <Util/TimedFilter.h>

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

	static constexpr float SAMPLE_MATCH_TOLERANCE = 0.001f; //???or relative to parameter space?

	enum { vA = 0, vB = 1, vC = 2, eBC = 0, eCA = 1, eAB = 2 }; // Triangle vertices and edges

	struct CTriangle
	{
		CSample* Samples[3] = {};
		U32      Adjacent[3] = { INVALID_INDEX, INVALID_INDEX, INVALID_INDEX };
		float    InvDenominator;
		float    ax, ay;
		float    abx, aby;
		float    acx, acy;
	};

	struct CEdge
	{
		U32 TriIndex = INVALID_INDEX;
		U32 EdgeIndex = INVALID_INDEX; // [0; 2]
		U32 Adjacent[2] = { INVALID_INDEX, INVALID_INDEX };
	};

	std::vector<CSample>   _Samples;
	std::vector<CTriangle> _Triangles;
	std::vector<CEdge>     _Contour;
	CPoseBuffer            _TmpPose;
	CAnimGraphNode*        _pActiveSamples[3] = { nullptr };
	float                  _Weights[3] = { 0.f };

	CStrID                 _XParamID;
	CStrID                 _YParamID;
	HVar                   _XParamHandle; // Cached for fast access
	HVar                   _YParamHandle; // Cached for fast access

	CTimedFilter<float>    _XFilter;
	CTimedFilter<float>    _YFilter;

	float                  _NormalizedTime = 0.f; // Current time normalized to [0..1]

public:

	CBlendSpace2D(CStrID XParamID, CStrID YParamID, float XSmoothTime = 0.f, float YSmoothTime = 0.f);

	virtual void  Init(CAnimationInitContext& Context) override;
	virtual void  Update(CAnimationUpdateContext& Context, float dt) override;
	virtual void  EvaluatePose(CPoseBuffer& Output) override;

	virtual float GetAnimationLengthScaled() const override;
	//!!!TODO: override IsActive!

	bool          AddSample(PAnimGraphNode&& Source, float XValue, float YValue);
};

}
