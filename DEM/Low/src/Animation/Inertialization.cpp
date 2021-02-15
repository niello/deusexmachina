#include "Inertialization.h"
#include <Animation/PoseBuffer.h>
#include <Math/Math.h> // FIXME: for PI, search in ACL/RTM?!

namespace DEM::Anim
{

//???space - local vs world? or always skip root bone inertialization and work in local space only?
//???support excluded bones like in UE4?
void CInertializationPoseDiff::Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt, float Duration)
{
	const auto BoneCount = CurrPose.size();
	_BoneDiffs.SetSize(BoneCount);
	n_assert_dbg(IsAligned16(_BoneDiffs.data()));
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		//!!!if PrevPose1[i] is invalid, continue;

		auto& BoneDiff = _BoneDiffs[i];
		const auto& CurrTfm = CurrPose[i];
		const auto& Prev1Tfm = PrevPose1[i];

		const auto Scale = acl::vector_sub(Prev1Tfm.scale, CurrTfm.scale);
		const float ScaleMagnitude = acl::vector_length3(Scale);
		if (ScaleMagnitude != 0.f)
			BoneDiff.ScaleAxis = acl::vector_div(Scale, acl::vector_set(ScaleMagnitude)); //???PERF: or vector_mul(Scale, 1.f / ScaleMagnitude)?

		const auto InvCurrRotation = acl::quat_conjugate(CurrTfm.rotation);
		const auto Rotation = acl::quat_mul(Prev1Tfm.rotation, InvCurrRotation);
		BoneDiff.RotationAxis = acl::quat_get_axis(Rotation);
		float RotationAngle = acl::quat_get_angle(Rotation); // [0; 2PI], then turn to [0; PI] by flipping axis direction
		if (RotationAngle > PI)
		{
			BoneDiff.RotationAxis = acl::vector_neg(BoneDiff.RotationAxis);
			RotationAngle = TWO_PI - RotationAngle;
		}

		const auto Translation = acl::vector_sub(Prev1Tfm.translation, CurrTfm.translation);
		const float TranslationMagnitude = acl::vector_length3(Translation);
		if (TranslationMagnitude != 0.f)
			BoneDiff.TranslationDir = acl::vector_div(Translation, acl::vector_set(TranslationMagnitude)); //???PERF: or vector_mul(v, 1.f / mag)?

		float ScaleSpeed = 0.f;
		float RotationSpeed = 0.f;
		float TranslationSpeed = 0.f;		
		if (dt > 0.f) //!!!and PrevPose2[i] is valid
		{
			const auto& Prev2Tfm = PrevPose2[i];

			if (ScaleMagnitude != 0.f)
			{
				const auto PrevScale = acl::vector_sub(Prev2Tfm.scale, CurrTfm.scale);
				const float PrevMagnitude = acl::vector_dot3(PrevScale, BoneDiff.ScaleAxis);
				ScaleSpeed = (ScaleMagnitude - PrevMagnitude) / dt;
			}

			if (RotationAngle != 0.f)
			{
				const auto PrevRotation = acl::quat_mul(Prev2Tfm.rotation, InvCurrRotation);

				// Get angle around an arbitrary axis
				// TODO: to routine
				const float Dot = acl::vector_dot3(BoneDiff.RotationAxis, PrevRotation);
				const float RawAngle = 2.f * std::atan2f(Dot, acl::quat_get_w(PrevRotation)); // [-2PI; 2PI], convert to [-PI; PI]
				const float PrevAngle = (RawAngle > PI) ? (RawAngle - TWO_PI) : (RawAngle < -PI) ? (RawAngle + TWO_PI) : RawAngle;

				RotationSpeed = n_normangle_signed_pi(RotationAngle - PrevAngle) / dt;
			}

			if (TranslationMagnitude != 0.f)
			{
				const auto PrevTranslation = acl::vector_sub(Prev2Tfm.translation, CurrTfm.translation);
				const float PrevMagnitude = acl::vector_dot3(Translation, BoneDiff.TranslationDir);
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
		Tfm.scale = acl::vector_mul_add(BoneDiff.ScaleAxis, BoneDiff.ScaleParams.Evaluate(ElapsedTime), Tfm.scale);
		Tfm.rotation = acl::quat_mul(acl::quat_from_axis_angle(BoneDiff.RotationAxis, BoneDiff.RotationParams.Evaluate(ElapsedTime)), Tfm.rotation);
		Tfm.translation = acl::vector_mul_add(BoneDiff.TranslationDir, BoneDiff.TranslationParams.Evaluate(ElapsedTime), Tfm.translation);
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

	if (Duration5 < std::numeric_limits<float>().epsilon())
	{
		_a = 0.f;
		_b = 0.f;
		_c = 0.f;
		_d = 0.f;
		_v0 = 0.f;
		_x0 = 0.f;
		_sign = 0.f;
	}
	else
	{
		const float v0_Dur = v0 * Duration;

		const float a0_Dur2 = std::max(0.0f, -8.f * v0_Dur - 20.f * x0);
		const float a0_Dur2_3 = a0_Dur2 * 3.f;
		const float a0 = a0_Dur2 / Duration2;

		_a = -0.5f * (a0_Dur2 + 6.f * v0_Dur + 12.f * x0) / Duration5;
		_b = 0.5f * (a0_Dur2_3 + 16.f * v0_Dur + 30.f * x0) / Duration4;
		_c = -0.5f * (a0_Dur2_3 + 12.f * v0_Dur + 20.f * x0) / Duration3;
		_d = 0.5f * a0;
		_v0 = v0;
		_x0 = x0;
	}
}
//---------------------------------------------------------------------

}
