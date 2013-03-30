#pragma once
#ifndef __DEM_L1_ANIM_CTLR_THIRD_PERSON_H__
#define __DEM_L1_ANIM_CTLR_THIRD_PERSON_H__

#include <Scene/AnimController.h>
#include <mathlib/polar.h>

// Animation controller, that implements logic of third-person camera.
// This controller updates transform only when it has changed, saving
// lots of recalculations. To force update use ForceNextUpdate().
// All angles are in radians.

namespace Scene
{

class CAnimControllerThirdPerson: public CAnimController
{
protected:

	polar2	Angles;
	float	Distance;

	float	MinVertAngle;
	float	MaxVertAngle;
	float	MinDistance;
	float	MaxDistance;

	bool	Dirty;

public:

	CAnimControllerThirdPerson();

	virtual bool	ApplyTo(Math::CTransformSRT& DestTfm);

	void			SetVerticalAngleLimits(float Min, float Max);
	void			SetDistanceLimits(float Min, float Max);
	void			SetAngles(float Vertical, float Horizontal);
	void			SetDistance(float Value);
	void			OrbitVertical(float Angle);
	void			OrbitHorizontal(float Angle);
	void			Zoom(float Amount);
	void			ForceNextUpdate() { Dirty = true; }
};

typedef Ptr<CAnimControllerThirdPerson> PAnimControllerThirdPerson;

inline CAnimControllerThirdPerson::CAnimControllerThirdPerson():
	Distance(1.f),
	MinVertAngle(0.f),
	MaxVertAngle(PI * 0.5f),
	MinDistance(0.01f),
	MaxDistance(10000.f),
	Dirty(true)
{
	Flags.Set(LocalSpace); // For now, later mb world space + offset + position of target node
	Channels.Set(Anim::Chnl_Translation | Anim::Chnl_Rotation);
}
//---------------------------------------------------------------------

inline void CAnimControllerThirdPerson::SetVerticalAngleLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinVertAngle = Min;
	MaxVertAngle = Max;
	Angles.theta = n_clamp(Angles.theta, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CAnimControllerThirdPerson::SetDistanceLimits(float Min, float Max)
{
	n_assert(Min <= Max);
	MinDistance = Min;
	MaxDistance = Max;
	Distance = n_clamp(Distance, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CAnimControllerThirdPerson::SetAngles(float Vertical, float Horizontal)
{
	Angles.theta = n_clamp(Vertical, MinVertAngle, MaxVertAngle);
	Angles.rho = Horizontal;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CAnimControllerThirdPerson::SetDistance(float Value)
{
	Distance = n_clamp(Value, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CAnimControllerThirdPerson::OrbitVertical(float Angle)
{
	Angles.theta = n_clamp(Angles.theta + Angle, MinVertAngle, MaxVertAngle);
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CAnimControllerThirdPerson::OrbitHorizontal(float Angle)
{
	Angles.rho += Angle;
	Dirty = true;
}
//---------------------------------------------------------------------

inline void CAnimControllerThirdPerson::Zoom(float Amount)
{
	Distance = n_clamp(Distance + Amount, MinDistance, MaxDistance);
	Dirty = true;
}
//---------------------------------------------------------------------

}

#endif
