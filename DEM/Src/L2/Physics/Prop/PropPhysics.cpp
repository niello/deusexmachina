#include "PropPhysics.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/Event/SetTransform.h>

//BEGIN_ATTRS_REGISTRATION(PropPhysics)
//	RegisterString(Physics, ReadOnly);
//	RegisterVector3(VelocityVector, ReadOnly);
//END_ATTRS_REGISTRATION

namespace Prop
{
__ImplementClass(Prop::CPropPhysics, 'PPHY', CPropAbstractPhysics);

using namespace Game;

CPropPhysics::~CPropPhysics()
{
	n_assert(!PhysicsEntity.IsValid());
}
//---------------------------------------------------------------------

void CPropPhysics::Activate()
{
    CPropAbstractPhysics::Activate();
	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropPhysics, OnSetTransform);
	PROP_SUBSCRIBE_PEVENT(OnMoveAfter, CPropPhysics, OnMoveAfter);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropPhysics, OnEntityRenamed);
}
//---------------------------------------------------------------------

void CPropPhysics::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnEntityRenamed);
	UNSUBSCRIBE_EVENT(OnMoveAfter);
	UNSUBSCRIBE_EVENT(SetTransform);
	CPropAbstractPhysics::Deactivate();
    PhysicsEntity = NULL;
}
//---------------------------------------------------------------------

void CPropPhysics::EnablePhysics()
{
	n_assert(!IsEnabled());

	nString PhysicsDesc;
	if (!GetEntity()->GetAttr<nString>(PhysicsDesc, CStrID("Physics"))) return;

	if (!PhysicsEntity.IsValid())
	{
		// Create and setup physics entity
		PhysicsEntity = CreatePhysicsEntity();
		n_assert(PhysicsEntity.IsValid());
		PhysicsEntity->CompositeName = GetEntity()->GetAttr<nString>(CStrID("Physics"));
		PhysicsEntity->SetUserData(GetEntity()->GetUID());
	}

	// Attach physics entity to physics level
	PhysicsEntity->SetTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));
	GetEntity()->GetLevel().GetPhysicsLevel()->AttachEntity(PhysicsEntity);

	PhysicsEntity->SetEnabled(true);

	// Call parent to do the real enable
	CPropAbstractPhysics::EnablePhysics();
}
//---------------------------------------------------------------------

void CPropPhysics::DisablePhysics()
{
	n_assert(IsEnabled());

	// Release the physics entity
	GetEntity()->GetLevel().GetPhysicsLevel()->RemoveEntity(GetPhysicsEntity()); // strange design
	CPropAbstractPhysics::DisablePhysics();
}
//---------------------------------------------------------------------

// Called after the physics subsystem has been triggered. This will transfer
// the physics entity's new transform back into the game entity.
bool CPropPhysics::OnMoveAfter(const Events::CEventBase& Event)
{
	if (IsEnabled() && PhysicsEntity->HasTransformChanged())
	{
		CPropAbstractPhysics::SetTransform(GetPhysicsEntity()->GetTransform());
		GetEntity()->SetAttr<vector3>(CStrID("VelocityVector"), PhysicsEntity->GetVelocity());
	}
	OK;
}
//---------------------------------------------------------------------

bool CPropPhysics::OnEntityRenamed(const Events::CEventBase& Event)
{
	if (PhysicsEntity.IsValid()) PhysicsEntity->SetUserData(GetEntity()->GetUID());
	OK;
}
//---------------------------------------------------------------------

void CPropPhysics::SetTransform(const matrix44& NewTF)
{
	// Forex EnvObjects can have no physics attached, in this case we behave as base class
	// Also we directly update transform for disabled PropPhysics
	//???what about Disabled & Locked physics entity?
	Physics::CEntity* pPhysEnt = GetPhysicsEntity();
	if (pPhysEnt) pPhysEnt->SetTransform(NewTF);
	CPropAbstractPhysics::SetTransform(NewTF);
}
//---------------------------------------------------------------------

Physics::CEntity* CPropPhysics::CreatePhysicsEntity()
{
	return Physics::CEntity::Create();
}
//---------------------------------------------------------------------

} // namespace Prop
