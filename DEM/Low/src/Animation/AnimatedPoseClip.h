#pragma once
#include <Animation/PoseClipBase.h>
#include <Animation/AnimationSampler.h>

// Pose clip that samples a pose from an animation player 

namespace DEM::Anim
{

class CAnimatedPoseClip : public CPoseClipBase
{
protected:

	PAnimationSampler _Player;

	// rename player to sampler
	// float start, end - normalized. If end > 1, it explicitly defines loop count
	// duration / speed, speed can be negative for reversed animation
	// loop flag here if required to create looping clip
	// all this is used to convert TL cip time to animation clip time

public:

	virtual void BindToOutput(const PPoseOutput& Output) override;
	virtual void PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
