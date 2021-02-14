#include "Inertialization.h"
#include <Animation/PoseBuffer.h>

namespace DEM::Anim
{
static constexpr float TIME_EPSILON = 1.e-7f;
static_assert(TIME_EPSILON * TIME_EPSILON * TIME_EPSILON * TIME_EPSILON * TIME_EPSILON > FLT_MIN, "Inertialization: too tiny TIME_EPSILON");

//???space - local vs world? or always skip root bone inertialization and work in local space only?
//???support excluded bones like in UE4?
void CInertializationPoseDiff::Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt)
{
	const auto BoneCount = CurrPose.size();
	_BoneDiffs.SetSize(BoneCount);
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		//!!!if PrevPose1[i] is invalid, continue;

		auto& BoneDiff = _BoneDiffs[i];
		const auto& CurrTfm = CurrPose[i];
		const auto& Prev1Tfm = PrevPose1[i];

		const auto Scale = Prev1Tfm.Scale - CurrTfm.Scale;
		BoneDiff.ScaleMagnitude = Scale.Length();
		if (BoneDiff.ScaleMagnitude != 0.f)
			BoneDiff.ScaleAxis = Scale / BoneDiff.ScaleMagnitude;

		auto InvCurrRotation = CurrTfm.Rotation;
		InvCurrRotation.invert();
		const auto Rotation = Prev1Tfm.Rotation * InvCurrRotation;
		BoneDiff.RotationAxis = Rotation.GetAxis();
		BoneDiff.RotationAngle = Rotation.GetAngle(); // [0; 2PI], then turn to [0; PI] by flipping axis direction
		if (BoneDiff.RotationAngle > PI)
		{
			BoneDiff.RotationAxis *= -1.f;
			BoneDiff.RotationAngle = TWO_PI - BoneDiff.RotationAngle;
		}

		const auto Translation = Prev1Tfm.Translation - CurrTfm.Translation;
		BoneDiff.TranslationMagnitude = Translation.Length();
		if (BoneDiff.TranslationMagnitude != 0.f)
			BoneDiff.TranslationDir = Translation / BoneDiff.TranslationMagnitude;

		//!!!if PrevPose2[i] is invalid or Prev1.DeltaTime is near 0.f, continue;

		const auto& Prev2Tfm = PrevPose2[i];

		if (BoneDiff.ScaleMagnitude != 0.f)
		{
			const auto PrevScale = Prev2Tfm.Scale - CurrTfm.Scale;
			const float PrevMagnitude = PrevScale.Dot(BoneDiff.ScaleAxis);
			BoneDiff.ScaleSpeed = (BoneDiff.ScaleMagnitude - PrevMagnitude) / dt;
		}

		if (BoneDiff.RotationAngle != 0.f)
		{
			const auto PrevRotation = Prev2Tfm.Rotation * InvCurrRotation;
			const float PrevAngle = PrevRotation.GetAngleAroundAxis(BoneDiff.RotationAxis);
			BoneDiff.RotationSpeed = n_normangle_signed_pi(BoneDiff.RotationAngle - PrevAngle) / dt;
		}

		if (BoneDiff.TranslationMagnitude != 0.f)
		{
			const auto PrevTranslation = Prev2Tfm.Translation - CurrTfm.Translation;
			const float PrevMagnitude = Translation.Dot(BoneDiff.TranslationDir);
			BoneDiff.TranslationSpeed = (BoneDiff.TranslationMagnitude - PrevMagnitude) / dt;
		}
	}
}
//---------------------------------------------------------------------

static float InertializeScalar(float x0, float v0, float t, float Duration)
{
	// If x0 < 0, mirror for conveniency
	const float sign = std::copysign(1.f, x0);
	if (sign < 0.f)
	{
		x0 = -x0;
		v0 = -v0;
	}

	if (v0 > 0.f)
	{
		// Avoid overshooting when velocity is directed away from the target value
		v0 = 0.f;
	}
	else if (v0 < 0.f)
	{
		// Clamp duration to ensure the curve is above zero for t = [0; Duration)
		Duration = std::min(Duration, -5.f * x0 / v0);
	}

	if ((Duration - t) <= TIME_EPSILON) return 0.f;

	const float Duration2 = Duration * Duration;
	const float Duration3 = Duration * Duration2;
	const float Duration4 = Duration * Duration3;
	const float Duration5 = Duration * Duration4;

	const float v0_Dur = v0 * Duration;

	const float a0_Dur2 = std::max(0.0f, -8.f * Duration * v0 - 20.f * x0);
	const float a0_Dur2_3 = a0_Dur2 * 3.f;
	const float a0 = a0_Dur2 / Duration2;

	const float A = -0.5f * (a0_Dur2 +  6.f * v0_Dur + 12.f * x0) / Duration5;
	const float B =  0.5f * (a0_Dur2_3 + 16.f * v0_Dur + 30.f * x0) / Duration4;
	const float C = -0.5f * (a0_Dur2_3 + 12.f * v0_Dur + 20.f * x0) / Duration3;
	const float D =  0.5f * a0;

	return sign * ((((((A * t) + B) * t + C) * t + D) * t + v0) * t + x0);
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::ApplyTo(CPoseBuffer& Target, float ElapsedTime, float Duration) const
{
	if (ElapsedTime < 0.f) ElapsedTime = 0.f;

	if ((Duration - ElapsedTime) <= TIME_EPSILON) return;

	const UPTR BoneCount = std::min(Target.size(), _BoneDiffs.size());
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		const auto& BoneDiff = _BoneDiffs[i];
		auto& Tfm = Target[i];

		Tfm.Translation += BoneDiff.TranslationDir *
			InertializeScalar(BoneDiff.TranslationMagnitude, BoneDiff.TranslationSpeed, ElapsedTime, Duration);

		// Apply the bone rotation difference
		quaternion Q;
		Q.set_rotate_axis_angle(BoneDiff.RotationAxis,
			InertializeScalar(BoneDiff.RotationAngle, BoneDiff.RotationSpeed, ElapsedTime, Duration));
		Tfm.Rotation = Q * Tfm.Rotation;

		// Apply the bone scale difference
		Tfm.Scale += BoneDiff.ScaleAxis *
			InertializeScalar(BoneDiff.ScaleMagnitude, BoneDiff.ScaleSpeed, ElapsedTime, Duration);
	}

	//Target.NormalizeRotations();
}
//---------------------------------------------------------------------

}
