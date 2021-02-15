#include "Inertialization.h"
#include <Animation/PoseBuffer.h>

namespace DEM::Anim
{

//???space - local vs world? or always skip root bone inertialization and work in local space only?
//???support excluded bones like in UE4?
void CInertializationPoseDiff::Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt, float Duration)
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
		const float ScaleMagnitude = Scale.Length();
		if (ScaleMagnitude != 0.f)
			BoneDiff.ScaleAxis = Scale / ScaleMagnitude;

		auto InvCurrRotation = CurrTfm.Rotation;
		InvCurrRotation.w *= -1.f; //InvCurrRotation.conjugate();
		const auto Rotation = Prev1Tfm.Rotation * InvCurrRotation;
		BoneDiff.RotationAxis = Rotation.GetAxis();
		float RotationAngle = Rotation.GetAngle(); // [0; 2PI], then turn to [0; PI] by flipping axis direction
		if (RotationAngle > PI)
		{
			BoneDiff.RotationAxis *= -1.f;
			RotationAngle = TWO_PI - RotationAngle;
		}

		const auto Translation = Prev1Tfm.Translation - CurrTfm.Translation;
		const float TranslationMagnitude = Translation.Length();
		if (TranslationMagnitude != 0.f)
			BoneDiff.TranslationDir = Translation / TranslationMagnitude;

		float ScaleSpeed = 0.f;
		float RotationSpeed = 0.f;
		float TranslationSpeed = 0.f;		
		if (dt > 0.f) //!!!and PrevPose2[i] is valid
		{
			const auto& Prev2Tfm = PrevPose2[i];

			if (ScaleMagnitude != 0.f)
			{
				const auto PrevScale = Prev2Tfm.Scale - CurrTfm.Scale;
				const float PrevMagnitude = PrevScale.Dot(BoneDiff.ScaleAxis);
				ScaleSpeed = (ScaleMagnitude - PrevMagnitude) / dt;
			}

			if (RotationAngle != 0.f)
			{
				const auto PrevRotation = Prev2Tfm.Rotation * InvCurrRotation;
				const float PrevAngle = PrevRotation.GetAngleAroundAxis(BoneDiff.RotationAxis);
				RotationSpeed = n_normangle_signed_pi(RotationAngle - PrevAngle) / dt;
			}

			if (TranslationMagnitude != 0.f)
			{
				const auto PrevTranslation = Prev2Tfm.Translation - CurrTfm.Translation;
				const float PrevMagnitude = Translation.Dot(BoneDiff.TranslationDir);
				TranslationSpeed = (TranslationMagnitude - PrevMagnitude) / dt;
			}
		}

		BoneDiff.ScaleParams.Prepare(ScaleMagnitude, ScaleSpeed, Duration);
		BoneDiff.RotationParams.Prepare(RotationAngle, RotationSpeed, Duration);
		BoneDiff.TranslationParams.Prepare(TranslationMagnitude, TranslationSpeed, Duration);
	}
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::ApplyTo(CPoseBuffer& Target, float ElapsedTime) const
{
	if (ElapsedTime < 0.f) ElapsedTime = 0.f;

	const UPTR BoneCount = std::min(Target.size(), _BoneDiffs.size());
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		const auto& BoneDiff = _BoneDiffs[i];
		auto& Tfm = Target[i];

		Tfm.Scale += BoneDiff.ScaleAxis * BoneDiff.ScaleParams.Evaluate(ElapsedTime);

		quaternion Q;
		Q.set_rotate_axis_angle(BoneDiff.RotationAxis, BoneDiff.RotationParams.Evaluate(ElapsedTime));
		Tfm.Rotation = Q * Tfm.Rotation;
		n_assert_dbg(n_fequal(Tfm.Rotation.magnitude(), 1.f));

		Tfm.Translation += BoneDiff.TranslationDir * BoneDiff.TranslationParams.Evaluate(ElapsedTime);
	}
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::CQuinticCurve::Prepare(float x0, float v0, float Duration)
{
	// If x0 < 0, mirror for conveniency
	_sign = std::copysign(1.f, x0);
	if (_sign < 0.f)
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

	const float Duration2 = Duration * Duration;
	const float Duration3 = Duration * Duration2;
	const float Duration4 = Duration * Duration3;
	const float Duration5 = Duration * Duration4;

	const float v0_Dur = v0 * Duration;

	const float a0_Dur2 = std::max(0.0f, -8.f * Duration * v0 - 20.f * x0);
	const float a0_Dur2_3 = a0_Dur2 * 3.f;
	const float a0 = a0_Dur2 / Duration2;

	_x0 = x0;
	_v0 = v0;
	_Duration = Duration;
	_a = -0.5f * (a0_Dur2 + 6.f * v0_Dur + 12.f * x0) / Duration5;
	_b = 0.5f * (a0_Dur2_3 + 16.f * v0_Dur + 30.f * x0) / Duration4;
	_c = -0.5f * (a0_Dur2_3 + 12.f * v0_Dur + 20.f * x0) / Duration3;
	_d = 0.5f * a0;
}
//---------------------------------------------------------------------

}
