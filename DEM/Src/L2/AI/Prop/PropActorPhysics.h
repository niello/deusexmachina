#pragma once
#ifndef __DEM_L2_PROP_ACTOR_PHYSICS_H__
#define __DEM_L2_PROP_ACTOR_PHYSICS_H__

#include <Physics/Prop/PropAbstractPhysics.h>

// Actor physical body interface. Listens for AI movement & align requests,
// responds to death, falling on the ground and other states.

namespace Physics
{
	typedef Ptr<class CCharEntity> PCharEntity;
}

namespace Prop
{

class CPropActorPhysics: public CPropAbstractPhysics
{
	__DeclareClass(CPropActorPhysics);

protected:

	Physics::PCharEntity PhysEntity;

	virtual void	EnablePhysics();
	virtual void	DisablePhysics();

	void			Stop();

	DECLARE_EVENT_HANDLER(OnMoveAfter, OnMoveAfter);
	DECLARE_EVENT_HANDLER(OnEntityRenamed, OnEntityRenamed);
	DECLARE_EVENT_HANDLER(AIBodyRequestLVelocity, OnRequestLinearVelocity);
	DECLARE_EVENT_HANDLER(AIBodyRequestAVelocity, OnRequestAngularVelocity);

	virtual void	SetTransform(const matrix44& NewTF);
	virtual void	OnRenderDebug();

public:

	virtual void Activate();
	virtual void Deactivate();

	virtual Physics::CEntity* GetPhysicsEntity() const;
};

}

#endif
