#pragma once
#ifndef __DEM_L2_PHYSICS_ENTITY_H__ //!!!to L1!
#define __DEM_L2_PHYSICS_ENTITY_H__

#include <Physics/Collision/Shape.h>
#include <Data/Flags.h>

// A physics entity is the front-end to a physics simulation object.

namespace Physics
{
using namespace Data;

class CLevel;
typedef Ptr<class CComposite> PComposite; //???all such things to PhysicsFwd.h?
typedef Ptr<class CShape> PShape; //???all such things to PhysicsFwd.h?

class CEntity: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CEntity);
	
public:

protected:
	
	static DWORD UIDCounter;

	DWORD			UID;
	CFlags			Flags;
	CLevel*			Level;				// currently attached to this Level
	PComposite		Composite;			// the Composite of this entity
	nArray<PShape>	CollidedShapes;
	matrix44		Transform;			// the backup Transform matrix
	//!!!don't store when no need! use ptrs & some matrix pool or allocation!
	matrix44		LockedTfm;			// backup Transform matrix when locked
	CStrID			UserData;
	uint			Stamp;

public:

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
	
	virtual void OnAttachedToLevel(CLevel* pLevel);
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

	bool			IsActive() const { return Flags.Is(PHYS_ENT_ACTIVE); }
	bool			IsLocked() const { return Flags.Is(PHYS_ENT_LOCKED); }
	bool			IsCollisionEnabled() const { return Flags.Is(PHYS_ENT_COLLISION_ENABLED); }
	bool			IsEnabled() const;
	bool			IsAttachedToLevel() const { return Level != NULL; }
	bool			IsHorizontalCollided() const;

	void			SetEnabled(bool Enabled);
	void			SetComposite(CComposite* pNew);
	CComposite*		GetComposite() const { return Composite.get_unsafe(); }
	DWORD			GetUID() const { return UID; }
	void			SetUserData(CStrID Data) { UserData = Data; }
	CStrID			GetUserData() const { return UserData; }
	void			SetStamp(uint s) { Stamp = s; }
	uint			GetStamp() const { return Stamp; }
	CLevel*			GetLevel() const { return Level; }
	int				GetNumCollisions() const;
	const nArray<Ptr<CShape>>& GetCollidedShapes() const { return CollidedShapes; }
};

RegisterFactory(CEntity);

typedef Ptr<CEntity> PEntity;

}

#endif
