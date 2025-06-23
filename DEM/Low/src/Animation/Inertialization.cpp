#include "Inertialization.h"
#include <Animation/PoseBuffer.h>
#include <Math/SIMDMath.h>
#include <Math/Math.h>
#include <rtm/packing/quatf.h> // quat_ensure_positive_w

// TODO: for PI, TWO_PI etc use rtm::constants::pi(), ...?

namespace DEM::Anim
{

static inline void RTM_SIMD_CALL PrepareCurves(rtm::vector4f x0, rtm::vector4f v0,
	rtm::vector4f Duration, CQuinticCurve4& Curves)
{
	// If x0 < 0, mirror for conveniency
	// TODO PERF: check if all_greater_equal check can optimize here
	const auto XSignMask = rtm::vector_greater_equal(x0, rtm::vector_zero());
	const auto sign = rtm::vector_select(XSignMask, rtm::vector_set(1.f), rtm::vector_set(-1.f));
	x0 = rtm::vector_mul(x0, sign);
	v0 = rtm::vector_mul(v0, sign);

	// Avoid overshooting when velocity is directed away from the target value
	// TODO PERF: can optimize duration clamping through v0 sign mask evaluation?
	v0 = rtm::vector_min(v0, rtm::vector_zero());

	// Clamp duration to ensure the curve is above zero for t = [0; Duration)
	const auto ClampedDuration = rtm::vector_min(Duration, rtm::vector_div(rtm::vector_mul(x0, -5.f), v0));
	const auto NonZeroVMask = rtm::vector_less_than(v0, rtm::vector_zero());
	Duration = rtm::vector_select(NonZeroVMask, ClampedDuration, Duration);

	static_assert(TINY * TINY * TINY * TINY * TINY > 0.f, "Inertialization: tiny float is too tiny");
	constexpr rtm::vector4f TINY_VECTOR{ TINY, TINY, TINY, TINY };
	const auto Mask = rtm::vector_less_than(Duration, TINY_VECTOR);
	const auto InvDuration = rtm::vector_select(Mask, rtm::vector_zero(), rtm::vector_reciprocal(Duration));
	// TODO PERF: check if all_less_than early exit gives any boost
	// TODO PERF: ensure the mask is not calculated twice. Use RTM for explicit mask reuse?

	const auto HalfInvDuration = rtm::vector_mul(InvDuration, 0.5f);
	const auto HalfInvDuration2 = rtm::vector_mul(InvDuration, HalfInvDuration);
	const auto HalfInvDuration3 = rtm::vector_mul(InvDuration, HalfInvDuration2);
	const auto HalfInvDuration4 = rtm::vector_mul(InvDuration, HalfInvDuration3);
	const auto HalfInvDuration5 = rtm::vector_mul(InvDuration, HalfInvDuration4);

	const auto v0_Dur = rtm::vector_mul(v0, Duration);
	const auto a0_Dur2 = rtm::vector_max(rtm::vector_zero(), rtm::vector_sub(rtm::vector_mul(v0_Dur, -8.f), rtm::vector_mul(x0, 20.f)));
	const auto a0_Dur2_3 = rtm::vector_mul(a0_Dur2, 3.f);

	Curves._Duration = Duration;
	Curves._a = rtm::vector_mul(rtm::vector_mul_add(x0, 12.f, rtm::vector_mul_add(v0_Dur, 6.f, a0_Dur2)), rtm::vector_neg(HalfInvDuration5));
	Curves._b = rtm::vector_mul(rtm::vector_mul_add(x0, 30.f, rtm::vector_mul_add(v0_Dur, 16.f, a0_Dur2_3)), HalfInvDuration4);
	Curves._c = rtm::vector_mul(rtm::vector_mul_add(x0, 20.f, rtm::vector_mul_add(v0_Dur, 12.f, a0_Dur2_3)), rtm::vector_neg(HalfInvDuration3));
	Curves._d = rtm::vector_mul(a0_Dur2, HalfInvDuration2);
	Curves._v0 = v0;
	Curves._x0 = x0;
	Curves._Sign = rtm::vector_select(Mask, rtm::vector_zero(), sign); // Mul on zero sign will effectively zero out the result
}
//---------------------------------------------------------------------

static DEM_FORCE_INLINE rtm::vector4f RTM_SIMD_CALL EvaluateCurves(rtm::vector4f_arg0 Time, const CQuinticCurve4& Curves)
{
	const auto t = rtm::vector_min(Time, Curves._Duration); // or mask = (Time >= _Duration) and set 0.f results by it
	auto Result = rtm::vector_mul_add(Curves._a, t, Curves._b);
	Result = rtm::vector_mul_add(Result, t, Curves._c);
	Result = rtm::vector_mul_add(Result, t, Curves._d);
	Result = rtm::vector_mul_add(Result, t, Curves._v0);
	Result = rtm::vector_mul_add(Result, t, Curves._x0);
	//!!!TODO PERF: can use sign mask instead of mul?!
	return rtm::vector_mul(Result, Curves._Sign);
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
		DEM_FORCE_INLINE operator rtm::vector4f() const { return _mm_load_ps(&v[0]); }
	};

	CFloat4A ScaleX0{};
	CFloat4A ScaleX1{};
	CFloat4A RotationQuatW0{};
	CFloat4A RotationQuatW1{};
	CFloat4A RotationPrevDot{};
	CFloat4A TranslationX0{};
	CFloat4A TranslationX1{};

	const auto VDuration = rtm::vector_set(Duration);
	const auto VInvDt = rtm::vector_reciprocal(rtm::vector_set(dt));
	const bool CalcSpeed = (dt > 0.f);

	const auto BoneCount = CurrPose.size();
	_Axes.SetSize(BoneCount);
	n_assert_dbg(Math::IsAligned<alignof(CBoneAxes)>(_Axes.data()));
	const auto TetradCount = (BoneCount + 3) / 4;
	_Curves.SetSize(TetradCount);
	n_assert_dbg(Math::IsAligned<alignof(CBoneCurves)>(_Curves.data()));
	for (UPTR i = 0, BoneIdx = 0; i < TetradCount; ++i)
	{
		for (UPTR j = 0; (j < 4) && (BoneIdx < BoneCount); ++j, ++BoneIdx)
		{
			//!!!if PrevPose1[i] is invalid or does not exist, continue;
			//(for vectorized can use validity mask or zero out x0! Or ignore at all? Invalid will not be processed in ApplyTo())

			auto& BoneAxes = _Axes[BoneIdx];
			const auto& CurrTfm = CurrPose[BoneIdx];
			const auto& Prev1Tfm = PrevPose1[BoneIdx];

			const auto Scale = rtm::vector_sub(Prev1Tfm.scale, CurrTfm.scale);
			ScaleX0[j] = rtm::vector_length_squared3(Scale);
			const bool HasScale = (ScaleX0[j] != 0.f);
			if (HasScale)
			{
				ScaleX0[j] = rtm::scalar_sqrt(ScaleX0[j]);
				BoneAxes.ScaleAxis = rtm::vector_div(Scale, rtm::vector_set(ScaleX0[j]));
			}

			const auto InvCurrRotation = rtm::quat_conjugate(CurrTfm.rotation);
			// Even an author of the math lib normalizes after quat_mul. There was an issue with vector_acos of value > 1.f below -> nan.
			// https://github.com/nfrechette/acl/blob/dadbbb3890da7c7c064ff11d212ad79da848fcb4/includes/acl/compression/impl/compact.transform.h#L598
			const auto Rotation = rtm::quat_normalize(rtm::quat_ensure_positive_w(rtm::quat_mul(Prev1Tfm.rotation, InvCurrRotation)));
			RotationQuatW0[j] = rtm::quat_get_w(Rotation);
			const bool HasRotation = (RotationQuatW0[j] < 1.f); // Angle = 0 when w = 1
			if (HasRotation)
			{
				// Get quaternion axis knowing that rotation is non-zero
				// NB: sqrt not vectorized because it is very fast and RotationAxis is used later in this loop
				const float QuatScale = rtm::scalar_sqrt(1.f - RotationQuatW0[j] * RotationQuatW0[j]);
				BoneAxes.RotationAxis = rtm::vector_div(Rotation, rtm::vector_set(QuatScale));
			}

			const auto Translation = rtm::vector_sub(Prev1Tfm.translation, CurrTfm.translation);
			TranslationX0[j] = rtm::vector_length_squared3(Translation);
			const bool HasTranslation = (TranslationX0[j] != 0.f);
			if (HasTranslation)
			{
				TranslationX0[j] = rtm::scalar_sqrt(TranslationX0[j]);
				BoneAxes.TranslationDir = rtm::vector_div(Translation, rtm::vector_set(TranslationX0[j]));
			}

			if (CalcSpeed) //!!!and PrevPose2[i] is valid and exists (for vectorized can use validity mask!)
			{
				const auto& Prev2Tfm = PrevPose2[BoneIdx];

				if (HasScale)
				{
					const auto PrevScale = rtm::vector_sub(Prev2Tfm.scale, CurrTfm.scale);
					ScaleX1[j] = rtm::vector_dot3(PrevScale, BoneAxes.ScaleAxis);
				}

				if (HasRotation)
				{
					// Remember data for calculation of PrevRotation twist angle around a RotationAxis
					const auto PrevRotation = rtm::quat_ensure_positive_w(rtm::quat_mul(Prev2Tfm.rotation, InvCurrRotation));
					RotationQuatW1[j] = rtm::quat_get_w(PrevRotation);
					RotationPrevDot[j] = rtm::vector_dot3(PrevRotation, BoneAxes.RotationAxis);
				}

				if (HasTranslation)
				{
					const auto PrevTranslation = rtm::vector_sub(Prev2Tfm.translation, CurrTfm.translation);
					TranslationX1[j] = rtm::vector_dot3(PrevTranslation, BoneAxes.TranslationDir);
				}
			}
		}

		// Get rotation angles from quat W. Deferred to vectorize acos.
		const auto RotationX0 = rtm::vector_mul(rtm::vector_acos(RotationQuatW0), 2.f); // [0; PI] due to positive w

		// Calculate speeds
		rtm::vector4f ScaleV0, RotationV0, TranslationV0;
		if (CalcSpeed)
		{
			const auto ScaleMask = rtm::vector_equal(ScaleX0, rtm::vector_zero());
			ScaleV0 = rtm::vector_select(ScaleMask, rtm::vector_zero(), rtm::vector_mul(rtm::vector_sub(ScaleX0, ScaleX1), VInvDt));

			const auto TranslationMask = rtm::vector_equal(TranslationX0, rtm::vector_zero());
			TranslationV0 = rtm::vector_select(TranslationMask, rtm::vector_zero(), rtm::vector_mul(rtm::vector_sub(TranslationX0, TranslationX1), VInvDt));

			// Get PrevRotation twist angle around a RotationAxis. Deferred to vectorize atan.
			// Instead of full atan2 logic handle w = 0 manually, otherwise w > 0.
			const auto Quat1ZeroWMask = rtm::vector_equal(RotationQuatW1, rtm::vector_zero());
			const auto SignedPi = rtm::vector_copy_sign(rtm::vector_set(PI), RotationPrevDot);
			const auto Quat1Angles = rtm::vector_mul(rtm::vector_atan(rtm::vector_div(RotationPrevDot, RotationQuatW1)), 2.f);
			const auto RotationX1 = rtm::vector_select(Quat1ZeroWMask, SignedPi, Quat1Angles); // [-PI; PI]
			auto AngleDiff = rtm::vector_sub(RotationX0, RotationX1); // [0; PI] - [-PI; PI] => [-PI; 2PI]

			// Normalize AngleDiff from [-PI; 2PI] to [-PI; PI]
			const auto AngleDiffGreaterPiMask = rtm::vector_greater_than(AngleDiff, rtm::vector_set(PI));
			AngleDiff = rtm::vector_select(AngleDiffGreaterPiMask, rtm::vector_sub(AngleDiff, rtm::vector_set(TWO_PI)), AngleDiff);

			const auto RotationMask = rtm::vector_equal(RotationX0, rtm::vector_zero());
			RotationV0 = rtm::vector_select(RotationMask, rtm::vector_zero(), rtm::vector_mul(AngleDiff, VInvDt));
		}
		else
		{
			ScaleV0 = rtm::vector_zero();
			RotationV0 = rtm::vector_zero();
			TranslationV0 = rtm::vector_zero();
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

	const auto VTime = rtm::vector_set(ElapsedTime);

	const UPTR BoneCount = std::min(Target.size(), _Axes.size());
	const UPTR TetradCount = (BoneCount + 3) / 4;
	for (UPTR i = 0, BoneIdx = 0; i < TetradCount; ++i)
	{
		const auto ScaleMagnitudes = EvaluateCurves(VTime, _Curves[i].Scale);
		const auto RotationHalfAngles = rtm::vector_mul(EvaluateCurves(VTime, _Curves[i].Rotation), 0.5f);
		const auto TranslationMagnitudes = EvaluateCurves(VTime, _Curves[i].Translation);

		rtm::vector4f VSin, VCos;
		rtm::vector_sincos(RotationHalfAngles, VSin, VCos);

		const auto& BoneAxes0 = _Axes[BoneIdx];
		const auto Quat0 = Math::vector_mix_xyza(rtm::vector_mul(rtm::vector_dup_x(VSin), BoneAxes0.RotationAxis), VCos);
		auto& Tfm0 = Target[BoneIdx];
		Tfm0.scale = rtm::vector_mul_add(BoneAxes0.ScaleAxis, rtm::vector_dup_x(ScaleMagnitudes), Tfm0.scale);
		Tfm0.rotation = rtm::quat_mul(Quat0, Tfm0.rotation);
		Tfm0.translation = rtm::vector_mul_add(BoneAxes0.TranslationDir, rtm::vector_dup_x(TranslationMagnitudes), Tfm0.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneAxes1 = _Axes[BoneIdx];
		const auto Quat1 = Math::vector_mix_xyzb(rtm::vector_mul(rtm::vector_dup_y(VSin), BoneAxes1.RotationAxis), VCos);
		auto& Tfm1 = Target[BoneIdx];
		Tfm1.scale = rtm::vector_mul_add(BoneAxes1.ScaleAxis, rtm::vector_dup_y(ScaleMagnitudes), Tfm1.scale);
		Tfm1.rotation = rtm::quat_mul(Quat1, Tfm1.rotation);
		Tfm1.translation = rtm::vector_mul_add(BoneAxes1.TranslationDir, rtm::vector_dup_y(TranslationMagnitudes), Tfm1.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneAxes2 = _Axes[BoneIdx];
		const auto Quat2 = Math::vector_mix_xyzc(rtm::vector_mul(rtm::vector_dup_z(VSin), BoneAxes2.RotationAxis), VCos);
		auto& Tfm2 = Target[BoneIdx];
		Tfm2.scale = rtm::vector_mul_add(BoneAxes2.ScaleAxis, rtm::vector_dup_z(ScaleMagnitudes), Tfm2.scale);
		Tfm2.rotation = rtm::quat_mul(Quat2, Tfm2.rotation);
		Tfm2.translation = rtm::vector_mul_add(BoneAxes2.TranslationDir, rtm::vector_dup_z(TranslationMagnitudes), Tfm2.translation);
		if (++BoneIdx >= BoneCount) break;

		const auto& BoneAxes3 = _Axes[BoneIdx];
		const auto Quat3 = Math::vector_mix_xyzd(rtm::vector_mul(rtm::vector_dup_w(VSin), BoneAxes3.RotationAxis), VCos);
		auto& Tfm3 = Target[BoneIdx];
		Tfm3.scale = rtm::vector_mul_add(BoneAxes3.ScaleAxis, rtm::vector_dup_w(ScaleMagnitudes), Tfm3.scale);
		Tfm3.rotation = rtm::quat_mul(Quat3, Tfm3.rotation);
		Tfm3.translation = rtm::vector_mul_add(BoneAxes3.TranslationDir, rtm::vector_dup_w(TranslationMagnitudes), Tfm3.translation);
		if (++BoneIdx >= BoneCount) break;
	}
}
//---------------------------------------------------------------------

}
