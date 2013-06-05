#pragma once
#ifndef __DEM_L2_PROP_PHYSICS_H__
#define __DEM_L2_PROP_PHYSICS_H__

#include <Game/Property.h>
#include <Physics/NodeControllerRigidBody.h>
#include <Physics/NodeAttrCollision.h>

// Allows to assign physics objects to the scene nodes of this entity. Objects can be
// dynamic (which control node transform) and kinematic (which only collide with dynamic ones).

namespace Prop
{
class CPropSceneNode;

class CPropPhysics: public Game::CProperty
{
	__DeclareClass(CPropPhysics);
	__DeclarePropertyStorage;

protected:

	nDictionary<Scene::CSceneNode*, Physics::PNodeControllerRigidBody>	Ctlrs; //???store node ptr in the controller?
	nArray<Physics::PNodeAttrCollision>									Attrs;

	void InitSceneNodeModifiers(CPropSceneNode& Prop);
	void TermSceneNodeModifiers(CPropSceneNode& Prop);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);

public:

	virtual void	Activate();
	virtual void	Deactivate();

	//!!!WRITE!
	void			GetAABB(bbox3& AABB) const { n_error("CPropPhysics::GetAABB() -> IMPLEMENT ME!!!"); }
};

}

#endif
