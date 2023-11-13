#pragma once
#include <Data/FixedArray.h>
#include <rtm/vector4f.h>

// Inertialization blending
// See:
// https://www.gdcvault.com/play/1025165/Inertialization
// https://www.gdcvault.com/play/1025331/Inertialization

namespace DEM::Anim
{
class CPoseBuffer;

// Four quintic curves vectorized
struct CQuinticCurve4
{
	rtm::vector4f _Duration;
	rtm::vector4f _a;
	rtm::vector4f _b;
	rtm::vector4f _c;
	rtm::vector4f _d;
	rtm::vector4f _v0;
	rtm::vector4f _x0;
	rtm::vector4f _Sign;
};

class CInertializationPoseDiff final
{
protected:

	struct CBoneAxes
	{
		rtm::vector4f ScaleAxis;
		rtm::vector4f RotationAxis;
		rtm::vector4f TranslationDir;

		DEM_ALLOCATE_ALIGNED(alignof(CBoneAxes));
	};

	struct CBoneCurves
	{
		CQuinticCurve4 Scale;
		CQuinticCurve4 Rotation;
		CQuinticCurve4 Translation;

		DEM_ALLOCATE_ALIGNED(alignof(CBoneCurves));
	};

	CFixedArray<CBoneAxes>   _Axes;   // Per bone
	CFixedArray<CBoneCurves> _Curves; // Per 4 bones

public:

	void Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt, float Duration);
	void ApplyTo(CPoseBuffer& Target, float ElapsedTime) const;
};

}
