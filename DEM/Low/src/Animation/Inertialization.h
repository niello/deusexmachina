#pragma once
#include <StdDEM.h>
#include <Math/TransformSRT.h>
#include <Data/FixedArray.h>

// Inertialization blending
// See:
// https://www.gdcvault.com/play/1025165/Inertialization
// https://www.gdcvault.com/play/1025331/Inertialization

namespace DEM::Anim
{
class CPoseBuffer;

class CInertializationPoseDiff final
{
protected:

	struct CQuinticCurve
	{
		float _x0;
		float _v0;
		float _sign;
		float _a;
		float _b;
		float _c;
		float _d;

		void Prepare(float x0, float v0, float Duration);

		DEM_FORCE_INLINE float Evaluate(float t) const
		{
			return _sign * ((((((_a * t) + _b) * t + _c) * t + _d) * t + _v0) * t + _x0);
		}
	};

	struct CBoneDiff
	{
		vector3       ScaleAxis;
		CQuinticCurve ScaleParams;

		vector3       RotationAxis;
		CQuinticCurve RotationParams;

		vector3       TranslationDir;
		CQuinticCurve TranslationParams;
	};

	CFixedArray<CBoneDiff> _BoneDiffs;

public:

	void Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt, float Duration);
	void ApplyTo(CPoseBuffer& Target, float ElapsedTime) const;
};

}
