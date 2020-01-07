#include "CameraController.h"

namespace Frame
{

void CCameraController::SetVerticalAngleLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinVertAngle = Min;
	MaxVertAngle = Max;
	Angles.Theta = std::clamp(Angles.Theta, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::SetDistanceLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinDistance = Min;
	MaxDistance = Max;
	Distance = std::clamp(Distance, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::SetVerticalAngle(float AngleRad)
{
	Angles.Theta = std::clamp(AngleRad, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::SetHorizontalAngle(float AngleRad)
{
	Angles.Phi = AngleRad;
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::SetDirection(const vector3& Dir)
{
	Angles.Set(Dir); //???or -Dir?
	Angles.Theta = std::clamp(Angles.Theta, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::SetDistance(float Value)
{
	Distance = std::clamp(Value, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::SetCOI(const vector3& NewCOI)
{
	if (COI == NewCOI) return;
	COI = NewCOI;
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::OrbitVertical(float Angle)
{
	if (Angle == 0.f) return;
	Angles.Theta = std::clamp(Angles.Theta + Angle, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::OrbitHorizontal(float Angle)
{
	if (Angle == 0.f) return;
	Angles.Phi += Angle;
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::Zoom(float Amount)
{
	if (Amount == 0.f) return;
	Distance = std::clamp(Distance + Amount, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::Move(const vector3& Translation)
{
	if (Translation == vector3::Zero) return;
	COI += Translation;
	Dirty = true;
}
//---------------------------------------------------------------------

//bool CCameraController::ApplyTo(Math::CTransformSRT& DestTfm)
//{
//	if (!Dirty) FAIL;
//
//	quaternion Qx, Qy;
//	Qx.set_rotate_x(-Angles.Theta);
//	Qy.set_rotate_y(-Angles.Phi);
//
//	// Optimized Qx * Qy. Z is negated to switch handedness to RH.
//	DestTfm.Rotation.set(Qx.x * Qy.w, Qx.w * Qy.y, -Qx.x * Qy.y, Qx.w * Qy.w);
//
//	DestTfm.Translation = COI + Angles.GetCartesianZ() * Distance;
//
//	Dirty = false;
//	OK;
//}
////---------------------------------------------------------------------

}
