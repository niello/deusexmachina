#pragma once
#ifndef __DEM_L2_PROP_CHARACTER_CTLR_H__
#define __DEM_L2_PROP_CHARACTER_CTLR_H__

#include <Game/Property.h>
#include <Physics/CharacterController.h>
#include <Physics/NodeControllerRigidBody.h>

// Special physics property that controls the root node of the entity by a character controller.
// Controller can be disabled, removing its body from the world, to switch to ragdoll.

namespace Prop
{
class CPropSceneNode;

class CPropCharacterController: public Game::CProperty //???derive from physics property?
{
	__DeclareClass(CPropCharacterController);
	__DeclarePropertyStorage;

protected:

	Physics::PNodeControllerRigidBody	NodeCtlr;
	Physics::PCharacterController		CharCtlr;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			CreateController();
	void			InitSceneNodeModifiers(CPropSceneNode& Prop);
	void			TermSceneNodeModifiers(CPropSceneNode& Prop);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(RequestLinearV, OnRequestLinearVelocity);
	DECLARE_EVENT_HANDLER(RequestAngularV, OnRequestAngularVelocity);
	DECLARE_EVENT_HANDLER(BeforePhysicsTick, OnPhysicsTick);
	DECLARE_EVENT_HANDLER(OnRenderDebug, OnRenderDebug);

public:

	bool			Enable();
	void			Disable();
	bool			IsEnabled() const { return NodeCtlr.IsValid() && NodeCtlr->IsActive(); }

	//!!!WRITE!
	void			GetAABB(bbox3& AABB) const { n_error("CPropCharacterController::GetAABB() -> IMPLEMENT ME!!!"); }
};

}

#endif
