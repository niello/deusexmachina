#pragma once
#include <Game/Property.h>
#include <Physics/CharacterController.h>
#include <Physics/NodeControllerRigidBody.h>

// Special physics property that controls the root node of the entity by a character controller.
// Controller can be disabled, removing its body from the world, to switch to ragdoll.

//???add ragdoll support here?

namespace Prop
{
class CPropSceneNode;

class CPropCharacterController: public Game::CProperty //???derive from physics property?
{
	FACTORY_CLASS_DECL;
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
	DECLARE_EVENT_HANDLER(BeforePhysicsTick, BeforePhysicsTick);
	DECLARE_EVENT_HANDLER(AfterPhysicsTick, AfterPhysicsTick);
	DECLARE_EVENT_HANDLER(OnRenderDebug, OnRenderDebug);
	DECLARE_EVENT_HANDLER(SetTransform, OnSetTransform);

public:

	bool			Enable();
	void			Disable();
	bool			IsEnabled() const { return NodeCtlr.IsValidPtr() && NodeCtlr->IsActive(); }

	Physics::CCharacterController* GetController() const { return CharCtlr.Get(); }
	//!!!WRITE!
	void			GetAABB(CAABB& AABB) const { Sys::Error("CPropCharacterController::GetAABB() -> IMPLEMENT ME!!!"); }
};

}
