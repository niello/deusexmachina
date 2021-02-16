#include "Inertialization.h"
#include <Animation/PoseBuffer.h>
#include <Math/Math.h> // FIXME: for PI, search in ACL/RTM?!

namespace DEM::Anim
{

////////////////!!!DBG TMP!///////////////////

//!!!DBG TMP!
ACL_FORCE_INLINE acl::Vector4_32 ACL_SIMD_CALL vector_select(acl::Vector4_32Arg0 mask, acl::Vector4_32Arg1 if_true, acl::Vector4_32Arg2 if_false) //RTM_NO_EXCEPT
{
	return _mm_or_ps(_mm_andnot_ps(mask, if_false), _mm_and_ps(if_true, mask));
}
//---------------------------------------------------------------------

inline acl::Vector4_32 ACL_SIMD_CALL vector_round_bankers(acl::Vector4_32Arg0 input) //RTM_NO_EXCEPT
{
	// SSE4
	//return _mm_round_ps(input, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);

	const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
	__m128 sign = _mm_and_ps(input, sign_mask);

	// We add the largest integer that a 32 bit floating point number can represent and subtract it afterwards.
	// This relies on the fact that if we had a fractional part, the new value cannot be represented accurately
	// and IEEE 754 will perform rounding for us. The default rounding mode is Banker's rounding.
	// This has the effect of removing the fractional part while simultaneously rounding.
	// Use the same sign as the input value to make sure we handle positive and negative values.
	const __m128 fractional_limit = _mm_set_ps1(8388608.0F); // 2^23
	__m128 truncating_offset = _mm_or_ps(sign, fractional_limit);
	__m128 integer_part = _mm_sub_ps(_mm_add_ps(input, truncating_offset), truncating_offset);

	// If our input was so large that it had no fractional part, return it unchanged
	// Otherwise return our integer part
	const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
	__m128 abs_input = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));
	__m128 is_input_large = _mm_cmpge_ps(abs_input, fractional_limit);
	return _mm_or_ps(_mm_and_ps(is_input_large, input), _mm_andnot_ps(is_input_large, integer_part));
}
//---------------------------------------------------------------------

inline void ACL_SIMD_CALL vector_sincos(acl::Vector4_32Arg0 input, acl::Vector4_32& outSin, acl::Vector4_32& outCos) //RTM_NO_EXCEPT
{
	// Use a degree 11 minimax approximation polynomial
	// See: GPGPU Programming for Games and Science (David H. Eberly)

	// Remap our input in the [-pi, pi] range
	__m128 quotient = _mm_mul_ps(input, _mm_set_ps1(1.591549430918953357688837633725143620e-01f)); // 1.f / 2PI
	quotient = vector_round_bankers(quotient);
	quotient = _mm_mul_ps(quotient, _mm_set_ps1(TWO_PI));
	__m128 x = _mm_sub_ps(input, quotient);

	// Remap our input in the [-pi/2, pi/2] range
	const __m128 sign_mask = _mm_set_ps(-0.0F, -0.0F, -0.0F, -0.0F);
	__m128 x_sign = _mm_and_ps(x, sign_mask);
	__m128 reference = _mm_or_ps(x_sign, _mm_set_ps1(PI));
	const __m128 reflection = _mm_sub_ps(reference, x);

	const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
	const __m128 x_abs = _mm_and_ps(x, _mm_castsi128_ps(abs_mask));
	__m128 is_less_equal_than_half_pi = _mm_cmple_ps(x_abs, _mm_set_ps1(HALF_PI));

	x = _mm_or_ps(_mm_andnot_ps(is_less_equal_than_half_pi, reflection), _mm_and_ps(x, is_less_equal_than_half_pi));

	// Calculate our value
	const __m128 x2 = _mm_mul_ps(x, x);

	// Sin
	__m128 sin = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(-2.3828544692960918e-8F)), _mm_set_ps1(2.7521557770526783e-6F));
	sin = _mm_add_ps(_mm_mul_ps(sin, x2), _mm_set_ps1(-1.9840782426250314e-4F));
	sin = _mm_add_ps(_mm_mul_ps(sin, x2), _mm_set_ps1(8.3333303183525942e-3F));
	sin = _mm_add_ps(_mm_mul_ps(sin, x2), _mm_set_ps1(-1.6666666601721269e-1F));
	sin = _mm_add_ps(_mm_mul_ps(sin, x2), _mm_set_ps1(1.0F));
	outSin = _mm_mul_ps(sin, x);

	// Cos
	__m128 cos = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(-2.6051615464872668e-7F)), _mm_set_ps1(2.4760495088926859e-5F));
	cos = _mm_add_ps(_mm_mul_ps(cos, x2), _mm_set_ps1(-1.3888377661039897e-3F));
	cos = _mm_add_ps(_mm_mul_ps(cos, x2), _mm_set_ps1(4.1666638865338612e-2F));
	cos = _mm_add_ps(_mm_mul_ps(cos, x2), _mm_set_ps1(-4.9999999508695869e-1F));
	cos = _mm_add_ps(_mm_mul_ps(cos, x2), _mm_set_ps1(1.0F));

	// Remap into [-pi, pi]
	outCos = _mm_or_ps(cos, _mm_andnot_ps(is_less_equal_than_half_pi, sign_mask));
}
//---------------------------------------------------------------------

//////////////////////////////////////////////

// TODO PERF: vectorize too?
static inline void PrepareQuinticSourceData(float& x0, float& v0, float& Duration, float& sign)
{
	// If x0 < 0, mirror for conveniency
	if (x0 < 0.f)
	{
		x0 = -x0;
		v0 = -v0;
		sign = -1.f;
	}
	else sign = 1.f;

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
			if (dt > 0.f) //!!!and PrevPose2[i] is valid and exists
			{
				const auto& Prev2Tfm = PrevPose2[BoneIdx];

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
		const auto RotationHalfAngles = acl::vector_mul(FourCurves.RotationParams.Evaluate(t), 0.5f);
		const auto TranslationMagnitudes = FourCurves.TranslationParams.Evaluate(t);

		// FIXME: use RTM functions!
		acl::Vector4_32 VSin, VCos;
		vector_sincos(RotationHalfAngles, VSin, VCos);

		const auto& BoneDiff0 = _BoneDiffs[BoneIdx];
		auto& Tfm0 = Target[BoneIdx];
		Tfm0.scale = acl::vector_mul_add(BoneDiff0.ScaleAxis, acl::vector_mix_xxxx(ScaleMagnitudes), Tfm0.scale);
		auto Quat0 = acl::vector_mul(acl::vector_mix_xxxx(VSin), BoneDiff0.RotationAxis);
		Quat0 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::A>(Quat0, VCos);
		Tfm0.rotation = acl::quat_mul(Quat0, Tfm0.rotation);
		Tfm0.translation = acl::vector_mul_add(BoneDiff0.TranslationDir, acl::vector_mix_xxxx(TranslationMagnitudes), Tfm0.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneDiff1 = _BoneDiffs[BoneIdx];
		auto& Tfm1 = Target[BoneIdx];
		Tfm1.scale = acl::vector_mul_add(BoneDiff1.ScaleAxis, acl::vector_mix_yyyy(ScaleMagnitudes), Tfm1.scale);
		auto Quat1 = acl::vector_mul(acl::vector_mix_yyyy(VSin), BoneDiff1.RotationAxis);
		Quat1 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::B>(Quat1, VCos);
		Tfm1.rotation = acl::quat_mul(Quat1, Tfm1.rotation);
		Tfm1.translation = acl::vector_mul_add(BoneDiff1.TranslationDir, acl::vector_mix_yyyy(TranslationMagnitudes), Tfm1.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneDiff2 = _BoneDiffs[BoneIdx];
		auto& Tfm2 = Target[BoneIdx];
		Tfm2.scale = acl::vector_mul_add(BoneDiff2.ScaleAxis, acl::vector_mix_zzzz(ScaleMagnitudes), Tfm2.scale);
		auto Quat2 = acl::vector_mul(acl::vector_mix_zzzz(VSin), BoneDiff2.RotationAxis);
		Quat2 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::C>(Quat2, VCos);
		Tfm2.rotation = acl::quat_mul(Quat2, Tfm2.rotation);
		Tfm2.translation = acl::vector_mul_add(BoneDiff2.TranslationDir, acl::vector_mix_zzzz(TranslationMagnitudes), Tfm2.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneDiff3 = _BoneDiffs[BoneIdx];
		auto& Tfm3 = Target[BoneIdx];
		Tfm3.scale = acl::vector_mul_add(BoneDiff3.ScaleAxis, acl::vector_mix_wwww(ScaleMagnitudes), Tfm3.scale);
		auto Quat3 = acl::vector_mul(acl::vector_mix_wwww(VSin), BoneDiff3.RotationAxis);
		Quat3 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::D>(Quat3, VCos);
		Tfm3.rotation = acl::quat_mul(Quat3, Tfm3.rotation);
		Tfm3.translation = acl::vector_mul_add(BoneDiff3.TranslationDir, acl::vector_mix_wwww(TranslationMagnitudes), Tfm3.translation);
		if (++BoneIdx >= BoneCount) break;
	}
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::CQuinticCurve4::Prepare(acl::Vector4_32 x0, acl::Vector4_32 v0, acl::Vector4_32 Duration, acl::Vector4_32 sign)
{
	static_assert(TINY * TINY * TINY * TINY * TINY > 0.f, "Inertialization: tiny float is too tiny");
	constexpr acl::Vector4_32 TINY_VECTOR{ TINY, TINY, TINY, TINY };
	const auto Mask = acl::vector_less_than(Duration, TINY_VECTOR);
	const auto InvDuration = vector_select(Mask, acl::vector_zero_32(), acl::vector_reciprocal(Duration));
	// TODO PERF: check if all_less_than early exit gives any boost
	// TODO PERF: ensure the mask is not calculated twice. Use RTM for explicit mask reuse?

	const auto HalfInvDuration = acl::vector_mul(InvDuration, 0.5f);
	const auto HalfInvDuration2 = acl::vector_mul(InvDuration, HalfInvDuration);
	const auto HalfInvDuration3 = acl::vector_mul(InvDuration, HalfInvDuration2);
	const auto HalfInvDuration4 = acl::vector_mul(InvDuration, HalfInvDuration3);
	const auto HalfInvDuration5 = acl::vector_mul(InvDuration, HalfInvDuration4);

	const auto v0_Dur = acl::vector_mul(v0, Duration);
	const auto a0_Dur2 = acl::vector_max(acl::vector_zero_32(), acl::vector_sub(acl::vector_mul(v0_Dur, -8.f), acl::vector_mul(x0, 20.f)));
	const auto a0_Dur2_3 = acl::vector_mul(a0_Dur2, 3.f);

	_a = acl::vector_mul(acl::vector_mul_add(x0, 12.f, acl::vector_mul_add(v0_Dur, 6.f, a0_Dur2)), acl::vector_neg(HalfInvDuration5));
	_b = acl::vector_mul(acl::vector_mul_add(x0, 30.f, acl::vector_mul_add(v0_Dur, 16.f, a0_Dur2_3)), HalfInvDuration4);
	_c = acl::vector_mul(acl::vector_mul_add(x0, 20.f, acl::vector_mul_add(v0_Dur, 12.f, a0_Dur2_3)), acl::vector_neg(HalfInvDuration3));
	_d = acl::vector_mul(a0_Dur2, HalfInvDuration2);
	_v0 = v0;
	_x0 = x0;
	_sign = vector_select(Mask, acl::vector_zero_32(), sign); // Mul on zero sign will effectively zero out the result
}
//---------------------------------------------------------------------

}
