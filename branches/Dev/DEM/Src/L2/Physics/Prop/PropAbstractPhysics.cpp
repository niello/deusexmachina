#include "PropAbstractPhysics.h"

#include <Game/Entity.h>
#include <Physics/Entity.h>
#include <Physics/Composite.h>
#include <Events/Subscription.h>
#include <Scene/PropSceneNode.h>
#include <Math/TransformSRT.h>
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

void CPropAbstractPhysics::SetTransform(const matrix44& NewTF)
{
	CPropSceneNode* pProp = GetEntity()->FindProperty<CPropSceneNode>();
	Physics::CEntity* pPhysEnt = GetPhysicsEntity();
	if (pPhysEnt && pProp && pProp->GetNode())
	{
		if (pProp->GetNode()->GetParent())
		{
			matrix44 InvParentPos;
			pProp->GetNode()->GetParent()->GetWorldMatrix().invert_simple(InvParentPos);
			pProp->GetNode()->SetLocalTransform(InvParentPos * NewTF);
		}
		else pProp->GetNode()->SetLocalTransform(NewTF);
	}

	CPropTransformable::SetTransform(NewTF);
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
