#pragma once
#ifndef __DEM_L2_PHYS_CONTACT_POINT_H__ //!!!to L1!
#define __DEM_L2_PHYS_CONTACT_POINT_H__

#include <mathlib/vector.h>
#include <Physics/MaterialTable.h>

// Contact points with position and Face-Up vector.

namespace Physics
{
class CEntity;
class CRigidBody;
class CComposite;

class CContactPoint
{
public:

	vector3			Position;
	vector3			UpVector;
	float			Depth;
	uint			EntityID;
	uint			RigidBodyID;
	CMaterialType	Material;

	CContactPoint();

	void		Clear();
	CEntity*	GetEntity() const;
	CRigidBody*	GetRigidBody() const;
};
//---------------------------------------------------------------------

//???need to init with defaults? or every time CP created physics fills it?
inline CContactPoint::CContactPoint():
	UpVector(0.0f, 1.0f, 0.0f),
	Depth(0.f),
	EntityID(0),
	RigidBodyID(0),
	Material(InvalidMaterial)
{
}
//---------------------------------------------------------------------

inline void CContactPoint::Clear()
{
	Position.set(0.0f, 0.0f, 0.0f);
	UpVector.set(0.0f, 1.0f, 0.0f);
	EntityID = 0;
	RigidBodyID = 0;
	Material = InvalidMaterial;
}
//---------------------------------------------------------------------

}

#endif
