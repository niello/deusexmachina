#pragma once
#include <Animation/PoseClipBase.h>
#include <Animation/AnimationSampler.h>

// Pose clip that samples a pose from an animation player 

namespace DEM::Anim
{
using PAnimatedPoseClip = std::unique_ptr<class CAnimatedPoseClip>;

class CAnimatedPoseClip : public CPoseClipBase
{
protected:

	CAnimationSampler _Sampler;

	// float start, end(?) - normalized. If end > 1, it explicitly defines loop count
	// duration / speed, speed can be negative for reversed animation
	// loop flag here if required to create looping clip
	// all this is used to convert TL cip time to animation clip time

public:

	void         SetAnimationClip(const PAnimationClip& Clip);

	virtual void BindToOutput(const PPoseOutput& Output) override;
	virtual void PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
