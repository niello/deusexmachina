#pragma once
#include <Data/FixedArray.h>
#include <acl/math/vector4_32.h>

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
	acl::Vector4_32 _Duration;
	acl::Vector4_32 _a;
	acl::Vector4_32 _b;
	acl::Vector4_32 _c;
	acl::Vector4_32 _d;
	acl::Vector4_32 _v0;
	acl::Vector4_32 _x0;
	acl::Vector4_32 _Sign;
};

class CInertializationPoseDiff final
{
protected:

	struct CBoneAxes
	{
		acl::Vector4_32 ScaleAxis;
		acl::Vector4_32 RotationAxis;
		acl::Vector4_32 TranslationDir;

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
