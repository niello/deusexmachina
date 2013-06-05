#pragma once
#ifndef __DEM_L1_MOTION_STATE_KINEMATIC_H__
#define __DEM_L1_MOTION_STATE_KINEMATIC_H__

#include <LinearMath/btMotionState.h>

// Motion state for kinematic objects. Gets transform every frame, but never or very
// rarely reads it back, so we store btTransform instead of quaternion + vector pair
// to avoid unnecessary conversions.

namespace Physics
{

class CMotionStateKinematic: public btMotionState
{
public:

	BT_DECLARE_ALIGNED_ALLOCATOR();

	btTransform Tfm;

	virtual void getWorldTransform(btTransform& worldTrans) const { worldTrans = Tfm; }
	virtual void setWorldTransform(const btTransform& worldTrans) {}
};

}

#endif
