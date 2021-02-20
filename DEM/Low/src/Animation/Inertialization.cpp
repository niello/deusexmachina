#include "Inertialization.h"
#include <Animation/PoseBuffer.h>
#include <Math/Math.h> // FIXME: for PI, search in ACL/RTM?! acl::k_pi_32, rtm::constants::pi() etc

namespace DEM::Anim
{

////////////////!!!DBG TMP!///////////////////
// Copypaste from https://github.com/nfrechette/rtm in wait for ACL 2.0 release

ACL_FORCE_INLINE acl::Vector4_32 ACL_SIMD_CALL vector_equal(acl::Vector4_32Arg0 lhs, acl::Vector4_32Arg1 rhs) //RTM_NO_EXCEPT
{
	return _mm_cmpeq_ps(lhs, rhs);
}
//---------------------------------------------------------------------

ACL_FORCE_INLINE acl::Vector4_32 ACL_SIMD_CALL vector_greater_than(acl::Vector4_32Arg0 lhs, acl::Vector4_32Arg1 rhs) //RTM_NO_EXCEPT
{
	return _mm_cmpgt_ps(lhs, rhs);
}
//---------------------------------------------------------------------

ACL_FORCE_INLINE acl::Vector4_32 ACL_SIMD_CALL vector_less_than(acl::Vector4_32Arg0 lhs, acl::Vector4_32Arg1 rhs) //RTM_NO_EXCEPT
{
	return _mm_cmplt_ps(lhs, rhs);
}
//---------------------------------------------------------------------

ACL_FORCE_INLINE acl::Vector4_32 ACL_SIMD_CALL vector_copy_sign(acl::Vector4_32Arg0 input, acl::Vector4_32Arg1 control_sign) //RTM_NO_EXCEPT
{
	const __m128 sign_bit = _mm_set_ps1(-0.0F);
	__m128 signs = _mm_and_ps(sign_bit, control_sign);
	__m128 abs_input = _mm_andnot_ps(sign_bit, input);
	return _mm_or_ps(abs_input, signs);
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

inline acl::Vector4_32 ACL_SIMD_CALL vector_acos(acl::Vector4_32Arg0 input) //RTM_NO_EXCEPT
{
	// Use the identity: acos(value) + asin(value) = PI/2
	// This ends up being: acos(value) = PI/2 - asin(value)
	// Since asin(value) = PI/2 - sqrt(1.0 - polynomial(value))
	// Our end result is acos(value) = sqrt(1.0 - polynomial(value))
	// This means we can re-use the same polynomial as asin()
	// See: GPGPU Programming for Games and Science (David H. Eberly)

	// We first calculate our scale: sqrt(1.0 - abs(value))
	// Use the sign bit to generate our absolute value since we'll re-use that constant
	const __m128 sign_bit = _mm_set_ps1(-0.0F);
	__m128 abs_value = _mm_andnot_ps(sign_bit, input);

	// Calculate our value
	__m128 result = _mm_add_ps(_mm_mul_ps(abs_value, _mm_set_ps1(-1.2690614339589956e-3F)), _mm_set_ps1(6.7072304676685235e-3F));
	result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-1.7162031184398074e-2F));
	result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(3.0961594977611639e-2F));
	result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-5.0207843052845647e-2F));
	result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(8.8986946573346160e-2F));
	result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(-2.1459960076929829e-1F));
	result = _mm_add_ps(_mm_mul_ps(result, abs_value), _mm_set_ps1(1.5707963267948966F));

	// Scale our result
	__m128 scale = _mm_sqrt_ps(_mm_sub_ps(_mm_set_ps1(1.0F), abs_value));
	result = _mm_mul_ps(result, scale);

	// Normally the math is as follow:
	// If input is positive: result
	// If input is negative: PI - result = -result + PI

	// As such, the offset is 0.0 when the input is positive and PI when negative
	__m128 is_input_negative = _mm_cmplt_ps(input, _mm_setzero_ps());
	__m128 offset = _mm_and_ps(is_input_negative, _mm_set_ps1(PI));

	// And our result has the same sign of the input
	__m128 input_sign = _mm_and_ps(input, sign_bit);
	result = _mm_or_ps(result, input_sign);
	return _mm_add_ps(result, offset);
}
//---------------------------------------------------------------------

inline acl::Vector4_32 ACL_SIMD_CALL vector_atan(acl::Vector4_32Arg0 input) //RTM_NO_EXCEPT
{
	// Use a degree 13 minimax approximation polynomial
	// See: GPGPU Programming for Games and Science (David H. Eberly)

	// Discard our sign, we'll restore it later
	const __m128i abs_mask = _mm_set_epi32(0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL, 0x7FFFFFFFULL);
	__m128 abs_value = _mm_and_ps(input, _mm_castsi128_ps(abs_mask));

	// Compute our value
	__m128 is_larger_than_one = _mm_cmpgt_ps(abs_value, _mm_set_ps1(1.0F));
	__m128 reciprocal = acl::vector_reciprocal(abs_value);

	__m128 x = acl::vector_blend(is_larger_than_one, reciprocal, abs_value);

	__m128 x2 = _mm_mul_ps(x, x);

	__m128 result = _mm_add_ps(_mm_mul_ps(x2, _mm_set_ps1(7.2128853633444123e-3F)), _mm_set_ps1(-3.5059680836411644e-2F));
	result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(8.1675882859940430e-2F));
	result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-1.3374657325451267e-1F));
	result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(1.9856563505717162e-1F));
	result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(-3.3324998579202170e-1F));
	result = _mm_add_ps(_mm_mul_ps(result, x2), _mm_set_ps1(1.0F));
	result = _mm_mul_ps(result, x);

	__m128 remapped = _mm_sub_ps(_mm_set_ps1(HALF_PI), result);

	// pi/2 - result
	result = acl::vector_blend(is_larger_than_one, remapped, result);

	// Keep the original sign
	return _mm_or_ps(result, _mm_and_ps(input, _mm_set_ps1(-0.0F)));
}
//---------------------------------------------------------------------

//////////////////////////////////////////////

static inline void ACL_SIMD_CALL PrepareCurves(acl::Vector4_32 x0, acl::Vector4_32 v0,
	acl::Vector4_32 Duration, CQuinticCurve4& Curves)
{
	// If x0 < 0, mirror for conveniency
	// TODO PERF: check if all_greater_equal check can optimize here
	const auto XSignMask = acl::vector_greater_equal(x0, acl::vector_zero_32());
	const auto sign = acl::vector_blend(XSignMask, acl::vector_set(1.f), acl::vector_set(-1.f));
	x0 = acl::vector_mul(x0, sign);
	v0 = acl::vector_mul(v0, sign);

	// Avoid overshooting when velocity is directed away from the target value
	// TODO PERF: can optimize duration clamping through v0 sign mask evaluation?
	v0 = acl::vector_min(v0, acl::vector_zero_32());

	// Clamp duration to ensure the curve is above zero for t = [0; Duration)
	const auto ClampedDuration = acl::vector_min(Duration, acl::vector_div(acl::vector_mul(x0, -5.f), v0));
	const auto NonZeroVMask = acl::vector_less_than(v0, acl::vector_zero_32());
	Duration = acl::vector_blend(NonZeroVMask, ClampedDuration, Duration);

	static_assert(TINY * TINY * TINY * TINY * TINY > 0.f, "Inertialization: tiny float is too tiny");
	constexpr acl::Vector4_32 TINY_VECTOR{ TINY, TINY, TINY, TINY };
	const auto Mask = acl::vector_less_than(Duration, TINY_VECTOR);
	const auto InvDuration = acl::vector_blend(Mask, acl::vector_zero_32(), acl::vector_reciprocal(Duration));
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

	Curves._Duration = Duration;
	Curves._a = acl::vector_mul(acl::vector_mul_add(x0, 12.f, acl::vector_mul_add(v0_Dur, 6.f, a0_Dur2)), acl::vector_neg(HalfInvDuration5));
	Curves._b = acl::vector_mul(acl::vector_mul_add(x0, 30.f, acl::vector_mul_add(v0_Dur, 16.f, a0_Dur2_3)), HalfInvDuration4);
	Curves._c = acl::vector_mul(acl::vector_mul_add(x0, 20.f, acl::vector_mul_add(v0_Dur, 12.f, a0_Dur2_3)), acl::vector_neg(HalfInvDuration3));
	Curves._d = acl::vector_mul(a0_Dur2, HalfInvDuration2);
	Curves._v0 = v0;
	Curves._x0 = x0;
	Curves._Sign = acl::vector_blend(Mask, acl::vector_zero_32(), sign); // Mul on zero sign will effectively zero out the result
}
//---------------------------------------------------------------------

static DEM_FORCE_INLINE acl::Vector4_32 ACL_SIMD_CALL EvaluateCurves(acl::Vector4_32Arg0 Time, const CQuinticCurve4& Curves)
{
	const auto t = acl::vector_min(Time, Curves._Duration); // or mask = (Time >= _Duration) and set 0.f results by it
	auto Result = acl::vector_mul_add(Curves._a, t, Curves._b);
	Result = acl::vector_mul_add(Result, t, Curves._c);
	Result = acl::vector_mul_add(Result, t, Curves._d);
	Result = acl::vector_mul_add(Result, t, Curves._v0);
	Result = acl::vector_mul_add(Result, t, Curves._x0);
	//!!!TODO PERF: can use sign mask instead of mul?!
	return acl::vector_mul(Result, Curves._Sign);
}
//---------------------------------------------------------------------

//???space - local vs world? or always skip root bone inertialization and work in local space only?
//???support excluded bones like in UE4?
void CInertializationPoseDiff::Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2, float dt, float Duration)
{
	// The sole purpose of this struct is to allow writing xyzw by index 0123.
	// Will RTM allow that with vector4f and quat4f without a switch-case?
	struct alignas(16) CFloat4A
	{
		float v[4];

		DEM_FORCE_INLINE float& operator[](UPTR i) { return v[i]; }
		DEM_FORCE_INLINE float operator[](UPTR i) const { return v[i]; }

		// FIXME: use RTM vector_load, otherwise is not portable! Aligned if possible!
		DEM_FORCE_INLINE operator acl::Vector4_32() const { return _mm_load_ps(&v[0]); }
	};

	CFloat4A ScaleX0{};
	CFloat4A ScaleX1{};
	CFloat4A RotationQuatW0{};
	CFloat4A RotationQuatW1{};
	CFloat4A RotationPrevDot{};
	CFloat4A TranslationX0{};
	CFloat4A TranslationX1{};

	const auto VDuration = acl::vector_set(Duration);
	const auto VInvDt = acl::vector_reciprocal(acl::vector_set(dt));
	const bool CalcSpeed = (dt > 0.f);

	const auto BoneCount = CurrPose.size();
	_Axes.SetSize(BoneCount);
	n_assert_dbg(IsAligned<alignof(CBoneAxes)>(_Axes.data()));
	const auto TetradCount = (BoneCount + 3) / 4;
	_Curves.SetSize(TetradCount);
	n_assert_dbg(IsAligned<alignof(CBoneCurves)>(_Curves.data()));
	for (UPTR i = 0, BoneIdx = 0; i < TetradCount; ++i)
	{
		for (UPTR j = 0; (j < 4) && (BoneIdx < BoneCount); ++j, ++BoneIdx)
		{
			//!!!if PrevPose1[i] is invalid or does not exist, continue;
			//(for vectorized can use validity mask or zero out x0! Or ignore at all? Invalid will not be processed in ApplyTo())

			auto& BoneAxes = _Axes[BoneIdx];
			const auto& CurrTfm = CurrPose[BoneIdx];
			const auto& Prev1Tfm = PrevPose1[BoneIdx];

			const auto Scale = acl::vector_sub(Prev1Tfm.scale, CurrTfm.scale);
			ScaleX0[j] = acl::vector_length_squared3(Scale);
			const bool HasScale = (ScaleX0[j] != 0.f);
			if (HasScale)
			{
				ScaleX0[j] = acl::sqrt(ScaleX0[j]);
				BoneAxes.ScaleAxis = acl::vector_div(Scale, acl::vector_set(ScaleX0[j]));
			}

			const auto InvCurrRotation = acl::quat_conjugate(CurrTfm.rotation);
			const auto Rotation = acl::quat_ensure_positive_w(acl::quat_mul(Prev1Tfm.rotation, InvCurrRotation));
			RotationQuatW0[j] = acl::quat_get_w(Rotation);
			const bool HasRotation = (RotationQuatW0[j] < 1.f); // Angle = 0 when w = 1
			if (HasRotation)
			{
				// Get quaternion axis knowing that rotation is non-zero
				// NB: sqrt not vectorized because it is very fast and RotationAxis is used later in this loop
				const float QuatScale = acl::sqrt(1.f - RotationQuatW0[j] * RotationQuatW0[j]);
				BoneAxes.RotationAxis = acl::vector_div(Rotation, acl::vector_set(QuatScale));
			}

			const auto Translation = acl::vector_sub(Prev1Tfm.translation, CurrTfm.translation);
			TranslationX0[j] = acl::vector_length_squared3(Translation);
			const bool HasTranslation = (TranslationX0[j] != 0.f);
			if (HasTranslation)
			{
				TranslationX0[j] = acl::sqrt(TranslationX0[j]);
				BoneAxes.TranslationDir = acl::vector_div(Translation, acl::vector_set(TranslationX0[j]));
			}

			if (CalcSpeed) //!!!and PrevPose2[i] is valid and exists (for vectorized can use validity mask!)
			{
				const auto& Prev2Tfm = PrevPose2[BoneIdx];

				if (HasScale)
				{
					const auto PrevScale = acl::vector_sub(Prev2Tfm.scale, CurrTfm.scale);
					ScaleX1[j] = acl::vector_dot3(PrevScale, BoneAxes.ScaleAxis);
				}

				if (HasRotation)
				{
					// Remember data for calculation of PrevRotation twist angle around a RotationAxis
					const auto PrevRotation = acl::quat_ensure_positive_w(acl::quat_mul(Prev2Tfm.rotation, InvCurrRotation));
					RotationQuatW1[j] = acl::quat_get_w(PrevRotation);
					RotationPrevDot[j] = acl::vector_dot3(PrevRotation, BoneAxes.RotationAxis);
				}

				if (HasTranslation)
				{
					const auto PrevTranslation = acl::vector_sub(Prev2Tfm.translation, CurrTfm.translation);
					TranslationX1[j] = acl::vector_dot3(PrevTranslation, BoneAxes.TranslationDir);
				}
			}
		}

		// Get rotation angles from quat W. Deferred to vectorize acos.
		const auto RotationX0 = acl::vector_mul(vector_acos(RotationQuatW0), 2.f); // [0; PI] due to positive w

		// Calculate speeds
		acl::Vector4_32 ScaleV0, RotationV0, TranslationV0;
		if (CalcSpeed)
		{
			const auto ScaleMask = vector_equal(ScaleX0, acl::vector_zero_32());
			ScaleV0 = acl::vector_blend(ScaleMask, acl::vector_zero_32(), acl::vector_mul(acl::vector_sub(ScaleX0, ScaleX1), VInvDt));

			const auto TranslationMask = vector_equal(TranslationX0, acl::vector_zero_32());
			TranslationV0 = acl::vector_blend(TranslationMask, acl::vector_zero_32(), acl::vector_mul(acl::vector_sub(TranslationX0, TranslationX1), VInvDt));

			// Get PrevRotation twist angle around a RotationAxis. Deferred to vectorize atan.
			// Instead of full atan2 logic handle w = 0 manually, otherwise w > 0.
			const auto Quat1ZeroWMask = vector_equal(RotationQuatW1, acl::vector_zero_32());
			const auto SignedPi = vector_copy_sign(acl::vector_set(PI), RotationPrevDot);
			const auto Quat1Angles = acl::vector_mul(vector_atan(acl::vector_div(RotationPrevDot, RotationQuatW1)), 2.f);
			const auto RotationX1 = acl::vector_blend(Quat1ZeroWMask, SignedPi, Quat1Angles); // [-PI; PI]
			auto AngleDiff = acl::vector_sub(RotationX0, RotationX1); // [0; PI] - [-PI; PI] => [-PI; 2PI]

			// Normalize AngleDiff from [-PI; 2PI] to [-PI; PI]
			const auto AngleDiffGreaterPiMask = vector_greater_than(AngleDiff, acl::vector_set(PI));
			AngleDiff = acl::vector_blend(AngleDiffGreaterPiMask, acl::vector_sub(AngleDiff, acl::vector_set(TWO_PI)), AngleDiff);

			const auto RotationMask = vector_equal(RotationX0, acl::vector_zero_32());
			RotationV0 = acl::vector_blend(RotationMask, acl::vector_zero_32(), acl::vector_mul(AngleDiff, VInvDt));
		}
		else
		{
			ScaleV0 = acl::vector_zero_32();
			RotationV0 = acl::vector_zero_32();
			TranslationV0 = acl::vector_zero_32();
		}

		PrepareCurves(ScaleX0, ScaleV0, VDuration, _Curves[i].Scale);
		PrepareCurves(RotationX0, RotationV0, VDuration, _Curves[i].Rotation);
		PrepareCurves(TranslationX0, TranslationV0, VDuration, _Curves[i].Translation);
	}
}
//---------------------------------------------------------------------

void CInertializationPoseDiff::ApplyTo(CPoseBuffer& Target, float ElapsedTime) const
{
	if (ElapsedTime < 0.f) ElapsedTime = 0.f;

	const auto VTime = acl::vector_set(ElapsedTime);

	const UPTR BoneCount = std::min(Target.size(), _Axes.size());
	const UPTR TetradCount = (BoneCount + 3) / 4;
	for (UPTR i = 0, BoneIdx = 0; i < TetradCount; ++i)
	{
		const auto ScaleMagnitudes = EvaluateCurves(VTime, _Curves[i].Scale);
		const auto RotationHalfAngles = acl::vector_mul(EvaluateCurves(VTime, _Curves[i].Rotation), 0.5f);
		const auto TranslationMagnitudes = EvaluateCurves(VTime, _Curves[i].Translation);

		// FIXME: use RTM functions! Will they have sincos?
		acl::Vector4_32 VSin, VCos;
		vector_sincos(RotationHalfAngles, VSin, VCos);

		const auto& BoneAxes0 = _Axes[BoneIdx];
		auto& Tfm0 = Target[BoneIdx];
		Tfm0.scale = acl::vector_mul_add(BoneAxes0.ScaleAxis, acl::vector_mix_xxxx(ScaleMagnitudes), Tfm0.scale);
		auto Quat0 = acl::vector_mul(acl::vector_mix_xxxx(VSin), BoneAxes0.RotationAxis);
		Quat0 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::A>(Quat0, VCos);
		Tfm0.rotation = acl::quat_mul(Quat0, Tfm0.rotation);
		Tfm0.translation = acl::vector_mul_add(BoneAxes0.TranslationDir, acl::vector_mix_xxxx(TranslationMagnitudes), Tfm0.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneAxes1 = _Axes[BoneIdx];
		auto& Tfm1 = Target[BoneIdx];
		Tfm1.scale = acl::vector_mul_add(BoneAxes1.ScaleAxis, acl::vector_mix_yyyy(ScaleMagnitudes), Tfm1.scale);
		auto Quat1 = acl::vector_mul(acl::vector_mix_yyyy(VSin), BoneAxes1.RotationAxis);
		Quat1 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::B>(Quat1, VCos);
		Tfm1.rotation = acl::quat_mul(Quat1, Tfm1.rotation);
		Tfm1.translation = acl::vector_mul_add(BoneAxes1.TranslationDir, acl::vector_mix_yyyy(TranslationMagnitudes), Tfm1.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneAxes2 = _Axes[BoneIdx];
		auto& Tfm2 = Target[BoneIdx];
		Tfm2.scale = acl::vector_mul_add(BoneAxes2.ScaleAxis, acl::vector_mix_zzzz(ScaleMagnitudes), Tfm2.scale);
		auto Quat2 = acl::vector_mul(acl::vector_mix_zzzz(VSin), BoneAxes2.RotationAxis);
		Quat2 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::C>(Quat2, VCos);
		Tfm2.rotation = acl::quat_mul(Quat2, Tfm2.rotation);
		Tfm2.translation = acl::vector_mul_add(BoneAxes2.TranslationDir, acl::vector_mix_zzzz(TranslationMagnitudes), Tfm2.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneAxes3 = _Axes[BoneIdx];
		auto& Tfm3 = Target[BoneIdx];
		Tfm3.scale = acl::vector_mul_add(BoneAxes3.ScaleAxis, acl::vector_mix_wwww(ScaleMagnitudes), Tfm3.scale);
		auto Quat3 = acl::vector_mul(acl::vector_mix_wwww(VSin), BoneAxes3.RotationAxis);
		Quat3 = acl::vector_mix<acl::VectorMix::X, acl::VectorMix::Y, acl::VectorMix::Z, acl::VectorMix::D>(Quat3, VCos);
		Tfm3.rotation = acl::quat_mul(Quat3, Tfm3.rotation);
		Tfm3.translation = acl::vector_mul_add(BoneAxes3.TranslationDir, acl::vector_mix_wwww(TranslationMagnitudes), Tfm3.translation);
		if (++BoneIdx >= BoneCount) break;
	}
}
//---------------------------------------------------------------------

}
