#pragma once
#include <Animation/PoseClipBase.h>
#include <Animation/AnimationBlender.h>

// Pose clip that blends poses from child pose tracks

namespace DEM::Anim
{
using PPoseTrack = std::unique_ptr<class CPoseTrack>;

class CCompositePoseClip : public CPoseClipBase
{
protected:

	CAnimationBlender       _Blender;
	std::vector<PPoseTrack> _Tracks;

public:

	// TODO: direct (evaluate weight curves, reuse priority), blendtree 1D, blendtree 2D etc
	virtual void          UpdateBlendParams() = 0;

	virtual PPoseClipBase Clone() const override;
	virtual void          BindToOutput(const PPoseOutput& Output) override;
	virtual void          PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
