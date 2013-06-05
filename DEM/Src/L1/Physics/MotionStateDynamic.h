#pragma once
#ifndef __DEM_L1_MOTION_STATE_DYNAMIC_H__
#define __DEM_L1_MOTION_STATE_DYNAMIC_H__

#include <LinearMath/btMotionState.h>

// Motion state for dynamic objects. Very rarely gets tfm, but reads it almost every frame, at least
// when object is active. Since it feeds the node controller that uses quaternion for rotation, and
// conversion will be done anyway, we store quaternion + vector here to reduce memory usage.

namespace Physics
{

class CMotionStateDynamic: public btMotionState
{
public:

	BT_DECLARE_ALIGNED_ALLOCATOR();

	btQuaternion	Rotation;
	btVector3		Position;
	bool			TfmChanged;

	CMotionStateDynamic(): TfmChanged(false) {}

	virtual void getWorldTransform(btTransform& worldTrans) const;
	virtual void setWorldTransform(const btTransform& worldTrans);
};

inline void CMotionStateDynamic::getWorldTransform(btTransform& worldTrans) const
{
	worldTrans.setRotation(Rotation);
	worldTrans.setOrigin(Position);
}
//---------------------------------------------------------------------

inline void CMotionStateDynamic::setWorldTransform(const btTransform& worldTrans)
{
	Rotation = worldTrans.getRotation();
	Position = worldTrans.getOrigin();
	TfmChanged = true;
}
//---------------------------------------------------------------------

}

#endif
