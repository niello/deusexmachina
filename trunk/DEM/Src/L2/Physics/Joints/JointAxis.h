#pragma once
#ifndef __DEM_L2_PHYSICS_JOINT_AXIS_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_JOINT_AXIS_H__

#include <kernel/ntypes.h>
#define dSINGLE
#include <ode/ode.h>

// Hold parameter definitions for a joint axis.

namespace Physics
{

class CJointAxis
{
public:

	vector3	Axis;
	bool	IsLoStopEnabled;
	bool	IsHiStopEnabled;
	float	Angle;
	float	LoStop;
	float	HiStop;
	float	Velocity;
	float	FMax;
	float	FudgeFactor;
	float	Bounce;
	float	CFM;
	float	StopERP;
	float	StopCFM;

	CJointAxis();
};
//---------------------------------------------------------------------

inline CJointAxis::CJointAxis():
	Axis(0.0f, 1.0f, 0.0f),
	IsLoStopEnabled(false),
	IsHiStopEnabled(false),
	Angle(0.0f),
	LoStop(0.0f),
	HiStop(0.0f),
	Velocity(0.0f),
	FMax(0.0f),
	FudgeFactor(1.0f),
	Bounce(0.0f),
	CFM(0.0f),
	StopERP(0.2f),
	StopCFM(0.0f)
{
}
//---------------------------------------------------------------------

}

#endif
