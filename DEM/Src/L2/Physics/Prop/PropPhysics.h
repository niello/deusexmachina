#pragma once
#ifndef __DEM_L2_PROP_PHYSICS_H__
#define __DEM_L2_PROP_PHYSICS_H__

#include "PropAbstractPhysics.h"
#include <Physics/Entity.h>

// A physics property adds basic physical behavior to a game entity.
// The default behavior is that of a passive physics object which will
// just passively roll and bounce around. Implement more advanced behavior
// in subclasses.
// Based on mangalore PhysicsProperty (C) 2005 Radon Labs GmbH

namespace Prop
{

class CPropPhysics: public CPropAbstractPhysics
{
	__DeclareClass(CPropPhysics);

protected:

	Physics::PEntity PhysicsEntity;

	virtual void				EnablePhysics();
	virtual void				DisablePhysics();
	virtual Physics::CEntity*	CreatePhysicsEntity();

	DECLARE_EVENT_HANDLER(OnMoveAfter, OnMoveAfter);
	DECLARE_EVENT_HANDLER(OnEntityRenamed, OnEntityRenamed);

	virtual void SetTransform(const matrix44& NewTF);

public:

	virtual ~CPropPhysics();

	virtual void Activate();
	virtual void Deactivate();

	virtual Physics::CEntity* GetPhysicsEntity() const { return PhysicsEntity.GetUnsafe(); }
};

} // namespace Prop

#endif
