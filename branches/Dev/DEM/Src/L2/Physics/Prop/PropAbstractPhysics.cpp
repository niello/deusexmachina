#include "PropAbstractPhysics.h"

#include <Physics/Entity.h>
#include <Physics/Composite.h>
#include <Events/Subscription.h>
#include <Loading/EntityFactory.h>

namespace Properties
{
ImplementRTTI(Properties::CPropAbstractPhysics, CPropTransformable);

CPropAbstractPhysics::~CPropAbstractPhysics()
{
	if (IsActive()) DisablePhysics(); //???is right?
}
//---------------------------------------------------------------------

// Called when property is attached to a game entity. This will create and
// setup the required physics entities.
void CPropAbstractPhysics::Activate()
{
	CPropTransformable::Activate();
	EnablePhysics();
}
//---------------------------------------------------------------------

// Called when property is going to be removed from its game entity.
// This will release the physics entity owned by the game entity.
void CPropAbstractPhysics::Deactivate()
{
	if (IsEnabled()) DisablePhysics();
	CPropTransformable::Deactivate();
}
//---------------------------------------------------------------------

void CPropAbstractPhysics::SetEnabled(bool Enable)
{
	if (Enabled != Enable)
	{
		if (Enable) EnablePhysics();
		else DisablePhysics();
	}
}
//---------------------------------------------------------------------

void CPropAbstractPhysics::EnablePhysics()
{
	n_assert(!IsEnabled());
	Enabled = true;
}
//---------------------------------------------------------------------

void CPropAbstractPhysics::DisablePhysics()
{
	n_assert(IsEnabled());
	Enabled = false;
}
//---------------------------------------------------------------------

void CPropAbstractPhysics::GetAABB(bbox3& AABB) const
{
	Physics::CEntity* pEnt = GetPhysicsEntity();
	if (pEnt && pEnt->GetComposite()) pEnt->GetComposite()->GetAABB(AABB);
	else
	{
		AABB.vmin.x = 
		AABB.vmin.y = 
		AABB.vmin.z = 
		AABB.vmax.x = 
		AABB.vmax.y = 
		AABB.vmax.z = 0.f;
	}
}
//---------------------------------------------------------------------

} // namespace Properties
