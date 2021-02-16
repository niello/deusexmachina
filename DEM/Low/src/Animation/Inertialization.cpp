#include "Inertialization.h"
#include <Animation/PoseBuffer.h>
#include <Math/Math.h> // FIXME: for PI, search in ACL/RTM?!

namespace DEM::Anim
{

// TODO PERF: vectorize too?
static inline void PrepareQuinticSourceData(float& x0, float& v0, float& Duration, float& sign)
{
	// If x0 < 0, mirror for conveniency
	sign = std::copysign(1.f, x0);
	if (x0 < 0.f)
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
}
//---------------------------------------------------------------------

//???space - local vs world? or always skip root bone inertialization and work in local space only?
//???support excluded bones like in UE4?
void CInertializationPoseDiff::Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt, float Duration)
{
	struct alignas(16) CFloat4A { float v[4]; };
	CFloat4A ScaleX0;
	CFloat4A ScaleV0;
	CFloat4A ScaleDurations;
	CFloat4A ScaleSigns;
	CFloat4A RotationX0;
	CFloat4A RotationV0;
	CFloat4A RotationDurations;
	CFloat4A RotationSigns;
	CFloat4A TranslationX0;
	CFloat4A TranslationV0;
	CFloat4A TranslationDurations;
	CFloat4A TranslationSigns;

	const auto BoneCount = CurrPose.size();
	_BoneDiffs.SetSize(BoneCount);
	n_assert_dbg(IsAligned16(_BoneDiffs.data()));
	const auto TetradCount = (BoneCount + 3) / 4;
	_Curves.SetSize(TetradCount);
	n_assert_dbg(IsAligned16(_Curves.data()));
	for (UPTR i = 0, BoneIdx = 0; i < TetradCount; ++i)
	{
		for (UPTR j = 0; (j < 4) && (BoneIdx < BoneCount); ++j, ++BoneIdx)
		{
			//!!!if PrevPose1[i] is invalid or does not exist, continue;

			auto& BoneDiff = _BoneDiffs[BoneIdx];
			const auto& CurrTfm = CurrPose[BoneIdx];
			const auto& Prev1Tfm = PrevPose1[BoneIdx];

			const auto Scale = acl::vector_sub(Prev1Tfm.scale, CurrTfm.scale);
			float ScaleMagnitude = acl::vector_length3(Scale);
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
			float TranslationMagnitude = acl::vector_length3(Translation);
			if (TranslationMagnitude != 0.f)
				BoneDiff.TranslationDir = acl::vector_div(Translation, acl::vector_set(TranslationMagnitude)); //???PERF: or vector_mul(v, 1.f / mag)?

			float ScaleSpeed = 0.f;
			float RotationSpeed = 0.f;
			float TranslationSpeed = 0.f;
			if (dt > 0.f) //!!!and PrevPose2[i] is valid and exists
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

			ScaleX0.v[j] = ScaleMagnitude;
			ScaleV0.v[j] = ScaleSpeed;
			ScaleDurations.v[j] = Duration;
			PrepareQuinticSourceData(ScaleX0.v[j], ScaleV0.v[j], ScaleDurations.v[j], ScaleSigns.v[j]);

			RotationX0.v[j] = RotationAngle;
			RotationV0.v[j] = RotationSpeed;
			RotationDurations.v[j] = Duration;
			PrepareQuinticSourceData(RotationX0.v[j], RotationV0.v[j], RotationDurations.v[j], RotationSigns.v[j]);

			TranslationX0.v[j] = TranslationMagnitude;
			TranslationV0.v[j] = TranslationSpeed;
			TranslationDurations.v[j] = Duration;
			PrepareQuinticSourceData(TranslationX0.v[j], TranslationV0.v[j], TranslationDurations.v[j], TranslationSigns.v[j]);
		}

		// FIXME: use RTM vector_load!
		_Curves[i].ScaleParams.Prepare(acl::vector_unaligned_load_32((uint8_t*)&ScaleX0.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&ScaleV0.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&ScaleDurations.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&ScaleSigns.v[0]));
		_Curves[i].RotationParams.Prepare(acl::vector_unaligned_load_32((uint8_t*)&RotationX0.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&RotationV0.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&RotationDurations.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&RotationSigns.v[0]));
		_Curves[i].TranslationParams.Prepare(acl::vector_unaligned_load_32((uint8_t*)&TranslationX0.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&TranslationV0.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&TranslationDurations.v[0]),
			acl::vector_unaligned_load_32((uint8_t*)&TranslationSigns.v[0]));

		//BoneDiff.ScaleParams.Prepare(ScaleMagnitude, ScaleSpeed, Dur, Sign);
		//BoneDiff.RotationParams.Prepare(RotationAngle, RotationSpeed, Dur, Sign);
		//BoneDiff.TranslationParams.Prepare(TranslationMagnitude, TranslationSpeed, Dur, Sign);
	}
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::ApplyTo(CPoseBuffer& Target, float ElapsedTime) const
{
	if (ElapsedTime < 0.f) ElapsedTime = 0.f;

	const auto t = acl::vector_set(ElapsedTime);

	const UPTR BoneCount = std::min(Target.size(), _BoneDiffs.size());
	const UPTR TetradCount = (BoneCount + 3) / 4;
	for (UPTR i = 0, BoneIdx = 0; i < TetradCount; ++i)
	{
		const auto& FourCurves = _Curves[i];
		const auto ScaleMagnitudes = FourCurves.ScaleParams.Evaluate(t);
		const auto RotationAngles = FourCurves.RotationParams.Evaluate(t);
		const auto TranslationMagnitudes = FourCurves.TranslationParams.Evaluate(t);

		// TODO PERF: calc vectorized sin and cos! 4 at a time! and use them to build quaternions!

		const auto& BoneDiff0 = _BoneDiffs[BoneIdx];
		auto& Tfm0 = Target[BoneIdx];
		Tfm0.scale = acl::vector_mul_add(BoneDiff0.ScaleAxis, acl::vector_mix_xxxx(ScaleMagnitudes), Tfm0.scale);
		Tfm0.rotation = acl::quat_mul(acl::quat_from_axis_angle(BoneDiff0.RotationAxis, acl::vector_get_x(RotationAngles)), Tfm0.rotation);
		Tfm0.translation = acl::vector_mul_add(BoneDiff0.TranslationDir, acl::vector_mix_xxxx(TranslationMagnitudes), Tfm0.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneDiff1 = _BoneDiffs[BoneIdx];
		auto& Tfm1 = Target[BoneIdx];
		Tfm1.scale = acl::vector_mul_add(BoneDiff1.ScaleAxis, acl::vector_mix_yyyy(ScaleMagnitudes), Tfm1.scale);
		Tfm1.rotation = acl::quat_mul(acl::quat_from_axis_angle(BoneDiff1.RotationAxis, acl::vector_get_y(RotationAngles)), Tfm1.rotation);
		Tfm1.translation = acl::vector_mul_add(BoneDiff1.TranslationDir, acl::vector_mix_yyyy(TranslationMagnitudes), Tfm1.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneDiff2 = _BoneDiffs[BoneIdx];
		auto& Tfm2 = Target[BoneIdx];
		Tfm2.scale = acl::vector_mul_add(BoneDiff2.ScaleAxis, acl::vector_mix_zzzz(ScaleMagnitudes), Tfm2.scale);
		Tfm2.rotation = acl::quat_mul(acl::quat_from_axis_angle(BoneDiff2.RotationAxis, acl::vector_get_z(RotationAngles)), Tfm2.rotation);
		Tfm2.translation = acl::vector_mul_add(BoneDiff2.TranslationDir, acl::vector_mix_zzzz(TranslationMagnitudes), Tfm2.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneDiff3 = _BoneDiffs[BoneIdx];
		auto& Tfm3 = Target[BoneIdx];
		Tfm3.scale = acl::vector_mul_add(BoneDiff3.ScaleAxis, acl::vector_mix_wwww(ScaleMagnitudes), Tfm3.scale);
		Tfm3.rotation = acl::quat_mul(acl::quat_from_axis_angle(BoneDiff3.RotationAxis, acl::vector_get_w(RotationAngles)), Tfm3.rotation);
		Tfm3.translation = acl::vector_mul_add(BoneDiff3.TranslationDir, acl::vector_mix_wwww(TranslationMagnitudes), Tfm3.translation);
		if (++BoneIdx >= BoneCount) break;
	}
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::CQuinticCurve::Prepare(float x0, float v0, float Duration, float sign)
{
	const float Duration2 = Duration * Duration;
	const float Duration3 = Duration * Duration2;
	const float Duration4 = Duration * Duration3;
	const float Duration5 = Duration * Duration4;

	if (Duration5 < TINY)
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
		_sign = sign;
	}
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::CQuinticCurve4::Prepare(acl::Vector4_32 x0, acl::Vector4_32 v0, acl::Vector4_32 Duration, acl::Vector4_32 sign)
{
	const auto Duration2 = acl::vector_mul(Duration, Duration);
	const auto Duration3 = acl::vector_mul(Duration, Duration2);
	const auto Duration4 = acl::vector_mul(Duration, Duration3);
	const auto Duration5 = acl::vector_mul(Duration, Duration4);

	const auto v0_Dur = acl::vector_mul(v0, Duration);

	const auto a0_Dur2 = acl::vector_max(acl::vector_zero_32(), acl::vector_sub(acl::vector_mul(v0_Dur, -8.f), acl::vector_mul(x0, 20.f)));
	const auto a0_Dur2_3 = acl::vector_mul(a0_Dur2, 3.f);
	const auto a0 = acl::vector_div(a0_Dur2, Duration2);

	_a = acl::vector_mul(acl::vector_mul_add(x0, 12.f, acl::vector_mul_add(v0_Dur, 6.f, a0_Dur2)), -0.5f);
	_a = acl::vector_div(_a, Duration5);
	_b = acl::vector_mul(acl::vector_mul_add(x0, 30.f, acl::vector_mul_add(v0_Dur, 16.f, a0_Dur2_3)), 0.5f);
	_b = acl::vector_div(_b, Duration4);
	_c = acl::vector_mul(acl::vector_mul_add(x0, 20.f, acl::vector_mul_add(v0_Dur, 12.f, a0_Dur2_3)), -0.5f);
	_c = acl::vector_div(_c, Duration3);
	_d = acl::vector_mul(a0, 0.5f);

	_v0 = v0;
	_x0 = x0;
	_sign = sign;

	// FIXME: can write better?
	// FIXME: ensure the mask is calculated once!
	// TODO: need vector_select! Exists only in RTM, not in ACL 1.3.5!
	/*
	constexpr acl::Vector4_32 TINY_VECTOR{ TINY, TINY, TINY, TINY };
	if (acl::vector_any_less_than(Duration5, TINY_VECTOR))
	{
		const auto Mask = acl::vector_less_than(Duration5, TINY_VECTOR);
		_a = acl::vector_select(Mask, acl::vector_zero_32(), _a);
		_b = acl::vector_select(Mask, acl::vector_zero_32(), _b);
		_c = acl::vector_select(Mask, acl::vector_zero_32(), _c);
		_d = acl::vector_select(Mask, acl::vector_zero_32(), _d);
		_v0 = acl::vector_select(Mask, acl::vector_zero_32(), _v0);
		_x0 = acl::vector_select(Mask, acl::vector_zero_32(), _x0);
		_sign = acl::vector_select(Mask, acl::vector_zero_32(), _sign);
	}
	*/
}
//---------------------------------------------------------------------

}
