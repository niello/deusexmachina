#pragma once
#include <Animation/PoseClipBase.h>
#include <Animation/AnimationPlayer.h>

// Pose clip that samples a pose from an animation player 

namespace DEM::Anim
{

class CAnimatedPoseClip : public CPoseClipBase
{
protected:

	PAnimationPlayer _Player;

public:

	virtual void BindToOutput(const PPoseOutput& Output) override;
	virtual void PlayInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
