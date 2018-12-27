#pragma once
#ifndef __DEM_L1_PHYSICS_BULLET_FWD_H__
#define __DEM_L1_PHYSICS_BULLET_FWD_H__

#include <Math/Matrix44.h>
#include <LinearMath/btTransform.h>

// Bullet physics library to DEM data convertors

//!!!later migrate to bullet math!
inline vector3 BtVectorToVector(const btVector3& Vec) { return vector3(Vec.x(), Vec.y(), Vec.z()); }
inline btVector3 VectorToBtVector(const vector3& Vec) { return btVector3(Vec.x, Vec.y, Vec.z); }

inline quaternion BtQuatToQuat(const btQuaternion& Q) { return quaternion(Q.x(), Q.y(), Q.z(), Q.w()); }
inline btQuaternion QuatToBtQuat(const quaternion& Q) { return btQuaternion(Q.x, Q.y, Q.z, Q.w); }

inline btTransform TfmToBtTfm(const matrix44& Tfm)
{
	vector3 AxisX = Tfm.AxisX();
	AxisX.norm();
	vector3 AxisY = Tfm.AxisY();
	AxisY.norm();
	vector3 AxisZ = Tfm.AxisZ();
	AxisZ.norm();
	return btTransform(
		btMatrix3x3(
			AxisX.x, AxisY.x, AxisZ.x,
			AxisX.y, AxisY.y, AxisZ.y,
			AxisX.z, AxisY.z, AxisZ.z),
		VectorToBtVector(Tfm.Translation()));
}

#endif