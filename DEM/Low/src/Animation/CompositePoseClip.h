#pragma once
#include <Animation/PoseClipBase.h>
#include <Animation/AnimationBlender.h>

// Pose clip that blends poses from child pose tracks

namespace DEM::Anim
{
class CAnimationBlender;
using PPoseTrack = std::unique_ptr<class CPoseTrack>;

class CCompositePoseClip : public CPoseClipBase
{
protected:

	CAnimationBlender       _Blender;
	std::vector<PPoseTrack> _Tracks;

public:

	// TODO: direct (evaluate weight curves, reuse priority), blendtree 1D, blendtree 2D etc
	virtual void UpdateBlendParams() = 0;

	virtual void PlayInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
