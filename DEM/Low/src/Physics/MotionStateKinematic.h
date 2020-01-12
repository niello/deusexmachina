#pragma once
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

	CMotionStateKinematic(): Tfm(btQuaternion(0.f, 0.f, 0.f, 1.f), btVector3(0.f, 0.f, 0.f)) {}

	virtual void getWorldTransform(btTransform& worldTrans) const { worldTrans = Tfm; }
	virtual void setWorldTransform(const btTransform& worldTrans) { /* must not be called on static and kinematic objects */ }
};

}
