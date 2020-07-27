#pragma once
#include <Animation/PoseClipBase.h>
#include <Animation/StaticPose.h>

// Pose clip that samples the same pose for its entire duration

namespace DEM::Anim
{

class CStaticPoseClip : public CPoseClipBase
{
protected:

	PStaticPose _Pose;

public:

	virtual void UpdateInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
