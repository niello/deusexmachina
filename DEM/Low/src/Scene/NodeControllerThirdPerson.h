#pragma once
#include <Math/Polar.h>
#include <System/System.h>

// Animation controller, that implements logic of third-person camera.
// This controller updates transform only when it has changed, saving
// lots of recalculations.
// NB: All angles are in radians.

namespace Scene
{

class CThirdPersonCameraController
{
protected:

	CPolar	Angles;
	vector3	COI;			// Center of interest, eye target in parent coordinates
	float	Distance = 1.f;

	float	MinVertAngle = 0.f;
	float	MaxVertAngle = PI * 0.5f;
	float	MinDistance = 0.01f;
	float	MaxDistance = 10000.f;

	bool	Dirty = true;

public:

	void			SetVerticalAngleLimits(float Min, float Max);
	void			SetDistanceLimits(float Min, float Max);

	void			SetVerticalAngle(float AngleRad);
	void			SetHorizontalAngle(float AngleRad);
	void			SetDirection(const vector3& Dir);
	void			SetDistance(float Value);
	void			SetCOI(const vector3& NewCOI);

	void			OrbitVertical(float AngleRad);
	void			OrbitHorizontal(float AngleRad);
	void			Zoom(float Amount);
	void			Move(const vector3& Translation);

	float			GetVerticalAngleMin() const { return MinVertAngle; }
	float			GetVerticalAngleMax() const { return MaxVertAngle; }
	float			GetDistanceMin() const { return MinDistance; }
	float			GetDistanceMax() const { return MaxDistance; }
	float           GetVerticalAngle() const {return Angles.Theta; }
	float           GetHorizontalAngle() const {return Angles.Phi; }
	float			GetDistance() const {return Distance; }
	const vector3&	GetCOI() const { return COI; }
};

inline void CThirdPersonCameraController::SetVerticalAngleLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinVertAngle = Min;
	MaxVertAngle = Max;
	Angles.Theta = std::clamp(Angles.Theta, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::SetDistanceLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinDistance = Min;
	MaxDistance = Max;
	Distance = std::clamp(Distance, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

void CThirdPersonCameraController::SetVerticalAngle(float AngleRad)
{
	Angles.Theta = std::clamp(AngleRad, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

void CThirdPersonCameraController::SetHorizontalAngle(float AngleRad)
{
	Angles.Phi = AngleRad;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::SetDirection(const vector3& Dir)
{
	Angles.Set(Dir); //???or -Dir?
	Angles.Theta = std::clamp(Angles.Theta, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::SetDistance(float Value)
{
	Distance = std::clamp(Value, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::SetCOI(const vector3& NewCOI)
{
	if (COI == NewCOI) return;
	COI = NewCOI;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::OrbitVertical(float Angle)
{
	if (Angle == 0.f) return;
	Angles.Theta = std::clamp(Angles.Theta + Angle, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::OrbitHorizontal(float Angle)
{
	if (Angle == 0.f) return;
	Angles.Phi += Angle;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::Zoom(float Amount)
{
	if (Amount == 0.f) return;
	Distance = std::clamp(Distance + Amount, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CThirdPersonCameraController::Move(const vector3& Translation)
{
	if (Translation == vector3::Zero) return;
	COI += Translation;
	Dirty = true;
}
//---------------------------------------------------------------------

}
