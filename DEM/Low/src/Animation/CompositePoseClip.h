#pragma once
#include <Animation/PoseClipBase.h>

// Pose clip that blends poses from child pose tracks

namespace DEM::Anim
{

class CCompositePoseClip : public CPoseClipBase
{
protected:

public:

	//virtual CalculateBlendParams() = 0; - direct (evaluate weight & priority curves), blendtree 1D, blendtree 2D etc

	virtual void PlayInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) override;
};

}
