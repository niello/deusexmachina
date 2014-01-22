#pragma once
#ifndef __DEM_L2_PROP_PHYSICS_H__
#define __DEM_L2_PROP_PHYSICS_H__

#include <Game/Property.h>
#include <Physics/NodeControllerRigidBody.h>
#include <Physics/NodeAttrCollision.h>

// Allows to assign physics objects to the scene nodes of this entity. Objects can be
// dynamic (which control node transform) and kinematic (which only collide with dynamic ones).

//???OnLevelSaving - save active state?
//!!!save velocities!

namespace Prop
{
class CPropSceneNode;

class CPropPhysics: public Game::CProperty
{
	__DeclareClass(CPropPhysics);
	__DeclarePropertyStorage;

protected:

	CArray<Physics::PNodeControllerRigidBody>	Ctlrs;
	CArray<Physics::PNodeAttrCollision>			Attrs;
	Physics::PRigidBody							RootBody; // Rigid body attached to a root node, velocity source

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			InitSceneNodeModifiers(CPropSceneNode& Prop);
	void			TermSceneNodeModifiers(CPropSceneNode& Prop);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(AfterPhysicsTick, AfterPhysicsTick);
	DECLARE_EVENT_HANDLER(SetTransform, OnSetTransform);

public:

	//!!!WRITE!
	void			GetAABB(CAABB& AABB) const { Core::Error("CPropPhysics::GetAABB() -> IMPLEMENT ME!!!"); }
};

}

#endif
