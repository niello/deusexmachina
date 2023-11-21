#include "CameraController.h"
#include <Scene/SceneNode.h>
#include <Math/SIMDMath.h>

namespace Frame
{

void CCameraController::SetNode(Scene::PSceneNode Node)
{
	_Node = Node;
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::Update(float dt)
{
	ZoneScoped;

	if (!_Node) return;

	// Screen edge moving (active zone in pixels & percents?)
	// Smoothing
	// Shaking
	// Collisions
	//???cache raycast for collision/occlusion handling?

	if (Dirty)
	{
		quaternion Qx, Qy;
		Qx.set_rotate_x(-Angles.Theta);
		Qy.set_rotate_y(-Angles.Phi);

		// Optimized Qx * Qy. Z is negated to switch handedness to RH.
		_Node->SetLocalRotation(rtm::quatf{ Qx.x * Qy.w, Qx.w * Qy.y, -Qx.x * Qy.y, Qx.w * Qy.w });
		_Node->SetWorldPosition(rtm::vector_mul_add(Angles.GetCartesianZ(), Distance, COI));

		Dirty = false;
	}
}
//---------------------------------------------------------------------

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

void CCameraController::SetCOI(const rtm::vector4f& NewCOI)
{
	if (rtm::vector_all_equal3(COI, NewCOI)) return;
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

void CCameraController::Move(const rtm::vector4f& Translation)
{
	if (rtm::vector_all_equal3(Translation, rtm::vector_zero())) return;
	COI = rtm::vector_add(COI, Translation);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::MoveForward(float Amount)
{
	if (Amount == 0.f) return;
	auto ForwardAxis = -vector3::AxisZ;
	ForwardAxis.rotate(vector3::AxisY, -Angles.Phi); // TODO: SIMD impl
	COI = rtm::vector_mul_add(Math::ToSIMD(ForwardAxis), Amount, COI);
	Dirty = true;
}
//---------------------------------------------------------------------

void CCameraController::MoveSide(float Amount)
{
	if (Amount == 0.f) return;
	auto RightAxis = vector3::AxisX;
	RightAxis.rotate(vector3::AxisY, -Angles.Phi); // TODO: SIMD impl
	COI = rtm::vector_mul_add(Math::ToSIMD(RightAxis), Amount, COI);
	Dirty = true;
}
//---------------------------------------------------------------------

}
