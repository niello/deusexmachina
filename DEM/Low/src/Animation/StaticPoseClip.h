#pragma once
#include <Animation/PoseClipBase.h>
#include <Animation/StaticPose.h>

// Pose clip that samples the same pose for its entire duration

namespace DEM::Anim
{
using PStaticPoseClip = std::unique_ptr<class CStaticPoseClip>;

class CStaticPoseClip : public CPoseClipBase
{
protected:

	PStaticPose            _Pose;
	std::unique_ptr<U16[]> _PortMapping;

public:

	virtual PPoseClipBase Clone() const override;
	virtual void          GatherSkeletonInfo(PSkeletonInfo& SkeletonInfo) override;
	virtual void          PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
