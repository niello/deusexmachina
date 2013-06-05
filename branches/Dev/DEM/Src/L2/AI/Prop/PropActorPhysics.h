#pragma once
#ifndef __DEM_L2_PROP_ACTOR_PHYSICS_H__
#define __DEM_L2_PROP_ACTOR_PHYSICS_H__

#include <Physics/Prop/PropTransformable.h>
#include <Physics/Entity.h>
#include <mathlib/bbox.h>

// Actor physical body interface. Listens for AI movement & align requests,
// responds to death, falling on the ground and other states.

namespace Physics
{
	class CEntity;
}

namespace Prop
{

class CPropActorPhysics: public CPropTransformable
{
	__DeclareClass(CPropActorPhysics);

protected:

	Physics::PEntity	PhysEntity;
	bool				Enabled;

	virtual void	EnablePhysics();
	virtual void	DisablePhysics();

	void			Stop();

	DECLARE_EVENT_HANDLER(AfterPhysics, AfterPhysics);
	DECLARE_EVENT_HANDLER(OnEntityRenamed, OnEntityRenamed);
	DECLARE_EVENT_HANDLER(AIBodyRequestLVelocity, OnRequestLinearVelocity);
	DECLARE_EVENT_HANDLER(AIBodyRequestAVelocity, OnRequestAngularVelocity);

	virtual void	SetTransform(const matrix44& NewTF);
	virtual void	OnRenderDebug();

public:

	CPropActorPhysics(): Enabled(false) {}
	virtual ~CPropActorPhysics();

	virtual void				Activate();
	virtual void				Deactivate();

	void						SetEnabled(bool Enable);
	bool						IsEnabled() const { return Enabled; }

	virtual Physics::CEntity*	GetPhysicsEntity() const { return PhysEntity; }
	void						GetAABB(bbox3& AABB) const;
};

}

#endif
