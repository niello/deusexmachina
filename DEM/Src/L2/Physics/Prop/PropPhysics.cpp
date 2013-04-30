#include "PropPhysics.h"

#include <Game/Entity.h>
#include <Physics/PhysicsServer.h>
#include <Physics/level.h>
#include <Physics/Event/SetTransform.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DefineString(Physics);
	DefineVector3(VelocityVector);
}

BEGIN_ATTRS_REGISTRATION(PropPhysics)
	RegisterString(Physics, ReadOnly);
	RegisterVector3(VelocityVector, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropPhysics, CPropAbstractPhysics);
ImplementFactory(Properties::CPropPhysics);
RegisterProperty(CPropPhysics);

using namespace Game;

CPropPhysics::~CPropPhysics()
{
	n_assert(!PhysicsEntity.isvalid());
}
//---------------------------------------------------------------------

void CPropPhysics::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropAbstractPhysics::GetAttributes(Attrs);
	Attrs.Append(Attr::VelocityVector);
	Attrs.Append(Attr::Physics);
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

	if (GetEntity()->Get<nString>(Attr::Physics).IsEmpty()) return;

	if (!PhysicsEntity.isvalid())
	{
		// Create and setup physics entity
		PhysicsEntity = CreatePhysicsEntity();
		n_assert(PhysicsEntity.isvalid());
		PhysicsEntity->CompositeName = GetEntity()->Get<nString>(Attr::Physics);
		PhysicsEntity->SetUserData(GetEntity()->GetUID());
	}

	// Attach physics entity to physics level
	PhysicsEntity->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	PhysicsSrv->GetLevel()->AttachEntity(PhysicsEntity);

	PhysicsEntity->SetEnabled(true);

	// Call parent to do the real enable
	CPropAbstractPhysics::EnablePhysics();
}
//---------------------------------------------------------------------

void CPropPhysics::DisablePhysics()
{
	n_assert(IsEnabled());

	// Release the physics entity
	PhysicsSrv->GetLevel()->RemoveEntity(GetPhysicsEntity()); // strange design
	CPropAbstractPhysics::DisablePhysics();
}
//---------------------------------------------------------------------

// Called after the physics subsystem has been triggered. This will transfer
// the physics entity's new transform back into the game entity.
bool CPropPhysics::OnMoveAfter(const CEventBase& Event)
{
	if (IsEnabled() && PhysicsEntity->HasTransformChanged())
	{
		CPropAbstractPhysics::SetTransform(GetPhysicsEntity()->GetTransform());
		GetEntity()->Set<vector3>(Attr::VelocityVector, PhysicsEntity->GetVelocity());
	}
	OK;
}
//---------------------------------------------------------------------

bool CPropPhysics::OnEntityRenamed(const CEventBase& Event)
{
	if (PhysicsEntity.isvalid()) PhysicsEntity->SetUserData(GetEntity()->GetUID());
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

} // namespace Properties
