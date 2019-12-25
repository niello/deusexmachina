#pragma once
#include <Game/Property.h>
#include <Physics/CollisionAttribute.h>
#include <Physics/RigidBody.h>

// Allows to assign physics objects to the scene nodes of this entity. Objects can be
// dynamic (which control node transform) and kinematic (which only collide with dynamic ones).

//???OnLevelSaving - save active state?
//!!!save velocities!

namespace Prop
{
class CPropSceneNode;

class CPropPhysics: public Game::CProperty
{
	FACTORY_CLASS_DECL;
	__DeclarePropertyStorage;

protected:

	CArray<Physics::PCollisionAttribute>	Attrs;
	Physics::PRigidBody						RootBody; // Rigid body attached to a root node, velocity source

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
	void			GetAABB(CAABB& AABB) const { Sys::Error("CPropPhysics::GetAABB() -> IMPLEMENT ME!!!"); }
};

}
