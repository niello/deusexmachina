#pragma once
#ifndef __DEM_L2_PHYSICS_ENTITY_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_ENTITY_H__

#include <Physics/Collision/Shape.h>
#include <Physics/MaterialTable.h>
#include <Data/Flags.h>

// A physics entity is the front-end to a physics simulation object.

namespace Physics
{
class CPhysWorldOld;
typedef Ptr<class CComposite> PComposite; //???all such things to PhysicsFwd.h?
typedef Ptr<class CShape> PShape; //???all such things to PhysicsFwd.h?

class CEntity: public Core::CRefCounted
{
	__DeclareClass(CEntity);
	
public:

protected:
	
	static DWORD UIDCounter;

	DWORD			UID;
	Data::CFlags	Flags;
	CPhysWorldOld*	Level;				// currently attached to this Level
	PComposite		Composite;			// the Composite of this entity
	nArray<PShape>	CollidedShapes;
	matrix44		Transform;			// the backup Transform matrix
	//!!!don't store when no need! use ptrs & some matrix pool or allocation!
	matrix44		LockedTfm;			// backup Transform matrix when locked
	CStrID			UserData;
	uint			Stamp;

	vector3			DesiredLinearVel;
	float			DesiredAngularVel;
	CMaterialType	GroundMtl;
	float			MaxHorizAccel;

public:

	float			Radius;
	float			Height;
	float			Hover;

	// Before adding flags check enums of derived classes
	enum
	{
		PHYS_ENT_ACTIVE				= 0x01,
		PHYS_ENT_LOCKED				= 0x02,
		PHYS_ENT_COLLISION_ENABLED	= 0x04
	};

	nString CompositeName;

	CEntity();
	virtual ~CEntity();

	virtual void Activate();
	virtual void Deactivate();
	
	virtual void OnAttachedToLevel(CPhysWorldOld* pLevel);
	virtual void OnRemovedFromLevel();
	virtual void OnStepBefore();
	virtual void OnStepAfter();
	virtual void OnFrameBefore();
	virtual void OnFrameAfter();
	virtual bool OnCollide(CShape* pCollidee);
	virtual void RenderDebug();

	virtual void		SetTransform(const matrix44& Tfm);
	virtual matrix44	GetTransform() const;
	virtual bool		HasTransformChanged() const;
	virtual vector3		GetVelocity() const;

	void			EnableCollision();
	void			DisableCollision();
	void			Lock();
	void			Unlock();
	void			Reset();

	void			SetDesiredLinearVelocity(const vector3& Velocity) { DesiredLinearVel = Velocity; }
	const vector3&	GetDesiredLinearVelocity() const { return DesiredLinearVel; }
	void			SetDesiredAngularVelocity(float Velocity) { DesiredAngularVel = Velocity; }
	float			GetDesiredAngularVelocity() const { return DesiredAngularVel; }
	CMaterialType	GetGroundMaterial() const { return GroundMtl; }

	bool			IsActive() const { return Flags.Is(PHYS_ENT_ACTIVE); }
	bool			IsLocked() const { return Flags.Is(PHYS_ENT_LOCKED); }
	bool			IsCollisionEnabled() const { return Flags.Is(PHYS_ENT_COLLISION_ENABLED); }
	bool			IsEnabled() const;
	bool			IsAttachedToLevel() const { return Level != NULL; }
	bool			IsHorizontalCollided() const;

	void			SetEnabled(bool Enabled);
	void			SetComposite(CComposite* pNew);
	CComposite*		GetComposite() const { return Composite.GetUnsafe(); }
	DWORD			GetUID() const { return UID; }
	void			SetUserData(CStrID Data) { UserData = Data; }
	CStrID			GetUserData() const { return UserData; }
	void			SetStamp(uint s) { Stamp = s; }
	uint			GetStamp() const { return Stamp; }
	CPhysWorldOld*	GetLevel() const { return Level; }
	int				GetNumCollisions() const;
	const nArray<Ptr<CShape>>& GetCollidedShapes() const { return CollidedShapes; }
};

typedef Ptr<CEntity> PEntity;

}

#endif
