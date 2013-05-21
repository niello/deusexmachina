#pragma once
#ifndef __DEM_L2_PHYSICS_SHAPE_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_SHAPE_H__

#include "CollideCategory.h"
#include <Physics/MaterialTable.h>
#include <Data/Params.h>
#include <mathlib/bbox.h>
#define dSINGLE
#include <ode/ode.h>

// Shapes are used in the physics subsystem for collision detection.

namespace Physics
{
class CEntity;
class CFilterSet;
class CContactPoint;

class CShape: public Core::CRefCounted
{
	__DeclareClassNoFactory;

public:

	enum EType
	{
		InvalidShapeType = 0,
		Box,
		Sphere,
		Plane,
		Capsule,
		Mesh,
		Heightfield,
		NumShapeTypes
	};

protected:

	enum { MaxContacts = 64 };

	friend class CRigidBody;
	friend class CPhysicsLevel;

	static nArray<CContactPoint>*	CollideContacts;
	static const CFilterSet*		CollideFilterSet;

	CEntity*		pEntity;
	EType			Type;
	matrix44		Transform;	//!!!MEMORY: too many mb redundant matrices in physics, check for optimization!
	matrix44		InitialTfm;
	CMaterialType	MaterialType;
	CRigidBody*		pRigidBody;
	int				CollCount;
	bool			IsHorizColl;
	uint			CatBits; //???!!!CFlags?!
	uint			CollBits; //???!!!CFlags?!
	dGeomID			ODEGeomID;	// the proxy geom id
	dMass			ODEMass;	// the mass structure
	dSpaceID		ODESpaceID;	// the collide space we're currently attached to

	void			AttachGeom(dGeomID GeomId, dSpaceID SpaceID);
	void			TransformMass();
	static CShape*	GetShapeFromGeom(dGeomID Geom) { return (CShape*)dGeomGetData(Geom); }
	vector4			GetDebugVisualizationColor() const;
	static void		ODENearCallback(void* data, dGeomID o1, dGeomID o2);

public:

	CShape(EType ShapeType);
	virtual ~CShape() = 0;

	virtual void	Init(Data::PParams Desc) = 0;
	virtual bool	Attach(dSpaceID SpaceID);
	virtual void	Detach();
	void			AttachToSpace(dSpaceID SpaceID);
	void			RemoveFromSpace();
	void			Collide(const CFilterSet& FilterSet, nArray<CContactPoint>& Contacts);
	virtual bool	OnCollide(CShape* pShape);
	virtual void	RenderDebug(const matrix44& ParentTfm) = 0;

	EType			GetType() const { return Type; }
	void			SetEntity(CEntity* pEnt) { pEntity = pEnt; }
	CEntity*		GetEntity() const { return pEntity; }
	void			GetAABB(bbox3& AABB) const;
	void			SetTransform(const matrix44& Tfm);
	const matrix44&	GetTransform() const { return Transform; }
	void			SetInitialTransform(const matrix44& Tfm) { InitialTfm = Tfm; }
	const matrix44&	GetInitialTransform() const { return InitialTfm; }
	void			SetMaterialType(CMaterialType MatType);
	CMaterialType	GetMaterialType() const { return MaterialType; }
	void			SetNumCollisions(int Num) { n_assert(Num >= 0); CollCount = Num; }
	int				GetNumCollisions() const { return CollCount; }
	void			SetHorizontalCollided(bool Is) { IsHorizColl = Is; }
	bool			IsHorizontalCollided() const { return IsHorizColl; }
	void			SetCategoryBits(uint NewCatBits);
	uint			GetCategoryBits() const { return CatBits; }
	void			SetCollideBits(uint NewCollBits);
	uint			GetCollideBits() const { return CollBits; }
	void			SetRigidBody(CRigidBody* pBody) { n_assert(!IsAttached()); pRigidBody = pBody; }
	CRigidBody*		GetRigidBody() const { return pRigidBody; }
	bool			IsAttached() const { return ODEGeomID != NULL; }
	dGeomID			GetGeomId() const { return ODEGeomID; }
	dSpaceID		GetSpaceId() const { return ODESpaceID; }
};
//---------------------------------------------------------------------

typedef Ptr<CShape> PShape;

inline void CShape::SetMaterialType(CMaterialType MatType)
{
	n_assert(!IsAttached());
	MaterialType = MatType;
}
//---------------------------------------------------------------------

inline void CShape::SetCategoryBits(uint NewCatBits)
{
	CatBits = NewCatBits;
	if (ODEGeomID) dGeomSetCategoryBits(ODEGeomID, CatBits);
}
//---------------------------------------------------------------------

inline void CShape::SetCollideBits(uint NewCollBits)
{
	CollBits = NewCollBits;
	if (ODEGeomID) dGeomSetCollideBits(ODEGeomID, CollBits);
}
//---------------------------------------------------------------------

}

#endif
