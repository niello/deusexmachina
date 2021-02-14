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

	struct CBoneDiff
	{
		vector3 ScaleAxis;
		float   ScaleMagnitude = 0.f;
		float   ScaleSpeed = 0.f;

		vector3 RotationAxis;
		float   RotationAngle = 0.f;
		float   RotationSpeed = 0.f;

		vector3 TranslationDir;
		float   TranslationMagnitude = 0.f;
		float   TranslationSpeed = 0.f;
	};

	CFixedArray<CBoneDiff> _BoneDiffs;

public:

	void Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt);
	void ApplyTo(CPoseBuffer& Target, float ElapsedTime, float Duration) const;
};

}
