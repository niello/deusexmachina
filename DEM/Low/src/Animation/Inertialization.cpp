#include "Inertialization.h"
#include <Animation/PoseBuffer.h>

namespace DEM::Anim
{
static constexpr float TIME_EPSILON = 1.e-7f;
static_assert(TIME_EPSILON* TIME_EPSILON* TIME_EPSILON* TIME_EPSILON* TIME_EPSILON > FLT_MIN, "Too tiny TIME_EPSILON");

//???space - local vs world? or always skip root bone inertialization and work in local space only?
//???support excluded bones like in UE4?
void CInertializationPoseDiff::Init(const CPoseBuffer& CurrPose, const CPoseBuffer& PrevPose1, const CPoseBuffer& PrevPose2)
{
	const auto BoneCount = CurrPose.size();
	_BoneDiffs.SetSize(BoneCount);
	for (UPTR i = 0; i < BoneCount; ++i)
	{
		auto& BoneDiff = _BoneDiffs[i];
		const auto& CurrTfm = CurrPose[i];
		const auto& Prev1Tfm = PrevPose1[i];
		const auto& Prev2Tfm = PrevPose2[i];

		//!!!if PrevPose1[i] is invalid, continue;

		//const bool Prev2IsValid = PrevPose2[i] is valid

		// FIXME: UE4 for reference, will be removed ASAP
		/*
		// Compute the bone translation difference
		{
			FVector TranslationDirection = FVector::ZeroVector;
			float TranslationMagnitude = 0.0f;
			float TranslationSpeed = 0.0f;

			const FVector T = Prev1Transform.GetTranslation() - PoseTransform.GetTranslation();
			TranslationMagnitude = T.Size();
			if (TranslationMagnitude > KINDA_SMALL_NUMBER)
			{
				TranslationDirection = T / TranslationMagnitude;
			}

			if (Prev2IsValid && Prev1.DeltaTime > KINDA_SMALL_NUMBER && TranslationMagnitude > KINDA_SMALL_NUMBER)
			{
				const FVector PrevT = Prev2Transform.GetTranslation() - PoseTransform.GetTranslation();
				const float PrevMagnitude = FVector::DotProduct(PrevT, TranslationDirection);
				TranslationSpeed = (TranslationMagnitude - PrevMagnitude) / Prev1.DeltaTime;
			}

			BoneDiff.TranslationDirection = TranslationDirection;
			BoneDiff.TranslationMagnitude = TranslationMagnitude;
			BoneDiff.TranslationSpeed = TranslationSpeed;
		}

		// Compute the bone rotation difference
		{
			FVector RotationAxis = FVector::ZeroVector;
			float RotationAngle = 0.0f;
			float RotationSpeed = 0.0f;

			const FQuat Q = Prev1Transform.GetRotation() * PoseTransform.GetRotation().Inverse();
			Q.ToAxisAndAngle(RotationAxis, RotationAngle);
			RotationAngle = FMath::UnwindRadians(RotationAngle);
			if (RotationAngle < 0.0f)
			{
				RotationAxis = -RotationAxis;
				RotationAngle = -RotationAngle;
			}

			if (Prev2IsValid && Prev1.DeltaTime > KINDA_SMALL_NUMBER && RotationAngle > KINDA_SMALL_NUMBER)
			{
				const FQuat PrevQ = Prev2Transform.GetRotation() * PoseTransform.GetRotation().Inverse();
				const float PrevAngle = PrevQ.GetTwistAngle(RotationAxis);
				RotationSpeed = FMath::UnwindRadians(RotationAngle - PrevAngle) / Prev1.DeltaTime;
			}

			BoneDiff.RotationAxis = RotationAxis;
			BoneDiff.RotationAngle = RotationAngle;
			BoneDiff.RotationSpeed = RotationSpeed;
		}

		// Compute the bone scale difference
		{
			FVector ScaleAxis = FVector::ZeroVector;
			float ScaleMagnitude = 0.0f;
			float ScaleSpeed = 0.0f;

			const FVector S = Prev1Transform.GetScale3D() - PoseTransform.GetScale3D();
			ScaleMagnitude = S.Size();
			if (ScaleMagnitude > KINDA_SMALL_NUMBER)
			{
				ScaleAxis = S / ScaleMagnitude;
			}

			if (Prev2IsValid && Prev1.DeltaTime > KINDA_SMALL_NUMBER && ScaleMagnitude > KINDA_SMALL_NUMBER)
			{
				const FVector PrevS = Prev2Transform.GetScale3D() - PoseTransform.GetScale3D();
				const float PrevMagnitude = FVector::DotProduct(PrevS, ScaleAxis);
				ScaleSpeed = (ScaleMagnitude - PrevMagnitude) / Prev1.DeltaTime;
			}

			BoneDiff.ScaleAxis = ScaleAxis;
			BoneDiff.ScaleMagnitude = ScaleMagnitude;
			BoneDiff.ScaleSpeed = ScaleSpeed;
		}
		*/
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
