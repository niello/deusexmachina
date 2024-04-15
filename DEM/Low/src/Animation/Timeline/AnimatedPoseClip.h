#pragma once
#include <Animation/Timeline/PoseClipBase.h>
#include <Animation/AnimationSampler.h>

// Pose clip that samples a pose from an animation player 

namespace DEM::Anim
{
using PAnimatedPoseClip = std::unique_ptr<class CAnimatedPoseClip>;

class alignas(CAnimationSampler) CAnimatedPoseClip : public CPoseClipBase
{
protected:

	CAnimationSampler      _Sampler; // At offset 0 for proper alignment
	std::unique_ptr<U16[]> _PortMapping;

	// float start, end(?) - normalized. If end > 1, it explicitly defines loop count
	// duration / speed, speed can be negative for reversed animation
	// loop flag here if required to create looping clip
	// all this is used to convert TL cip time to animation clip time

public:

	DEM_ALLOCATE_ALIGNED(alignof(CAnimatedPoseClip));

	void                  SetAnimationClip(const PAnimationClip& Clip);

	virtual PPoseClipBase Clone() const override;
	virtual void          GatherSkeletonInfo(PSkeletonInfo& SkeletonInfo) override;
	virtual void          PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
