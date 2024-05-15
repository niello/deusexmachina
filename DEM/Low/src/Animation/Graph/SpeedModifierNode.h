#pragma once
#include <Animation/Graph/AnimGraphNode.h>
#include <Data/StringID.h>

// Modifies playback speed for the underlying subgraph

namespace DEM::Anim
{
using PSpeedModifierNode = std::unique_ptr<class CSpeedModifierNode>;

class CSpeedModifierNode : public CAnimGraphNode
{
protected:

	PAnimGraphNode _Subgraph;
	CStrID         _ParamID;
	UPTR           _ParamIndex = INVALID_INDEX; // Cached for fast access
	float          _FallbackMultiplier = 1.f;   // If param is not found, use this value

public:

	CSpeedModifierNode(PAnimGraphNode&& Subgraph, CStrID ParamID, float FallbackMultiplier = 1.f);
	CSpeedModifierNode(PAnimGraphNode&& Subgraph, float Multiplier);

	virtual void Init(CAnimationInitContext& Context) override;
	virtual void Update(CAnimationUpdateContext& Context, float dt) override;
	virtual void EvaluatePose(CPoseBuffer& Output) override;

	virtual float GetAnimationLengthScaled() const override { return _Subgraph ? _Subgraph->GetAnimationLengthScaled() : 0.f; }
	virtual bool  IsActive() const override { return _Subgraph && _Subgraph->IsActive(); }
};

}
