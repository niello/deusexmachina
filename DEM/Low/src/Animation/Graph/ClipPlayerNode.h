#pragma once
#include <Animation/Graph/AnimGraphNode.h>
#include <Animation/AnimationSampler.h>
#include <Data/StringID.h>

// Animation graph node that plays a clip resource

namespace DEM::Anim
{
using PClipPlayerNode = std::unique_ptr<class CClipPlayerNode>;

class alignas(CAnimationSampler) CClipPlayerNode : public CAnimGraphNode
{
protected:

	//!!!TODO: support composite clips in a sampler!
	CAnimationSampler      _Sampler; // At offset 0 for proper alignment
	std::unique_ptr<U16[]> _PortMapping;

	CStrID                 _ClipID;

	float                  _CurrClipTime = 0.f;
	U32                    _LastUpdateIndex = 0;

	float                  _StartTime = 0.f; // In a clip space, not modified by a playback speed
	float                  _Speed = 1.f;
	bool                   _Loop = true;
	bool                   _ResetOnActivate = true;

	void ResetTime();

public:

	DEM_ALLOCATE_ALIGNED(alignof(CClipPlayerNode));

	CClipPlayerNode(CStrID ClipID, bool Loop = true, float Speed = 1.f, float StartTime = 0.f);

	virtual void Init(CAnimationInitContext& Context) override;
	virtual void Update(CAnimationUpdateContext& Context, float dt) override;
	virtual void EvaluatePose(CPoseBuffer& Output) override;

	virtual float GetAnimationLengthScaled() const override;
	virtual bool  IsActive() const override;
};

}
