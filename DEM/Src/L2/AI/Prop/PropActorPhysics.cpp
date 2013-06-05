#include "PropActorPhysics.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Physics/PhysicsWorldOld.h>
#include <Physics/Composite.h>
#include <Render/DebugDraw.h>

namespace Prop
{
__ImplementClass(Prop::CPropActorPhysics, 'PRAP', Prop::CPropTransformable);

CPropActorPhysics::~CPropActorPhysics()
{
	if (IsActive()) DisablePhysics(); //???is right?
}
//---------------------------------------------------------------------

void CPropActorPhysics::Activate()
{
	CPropTransformable::Activate();

	PhysEntity = Physics::CEntity::CreateInstance();
	PhysEntity->SetUserData(GetEntity()->GetUID());
	PhysEntity->SetTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));
	PhysEntity->CompositeName = GetEntity()->GetAttr<nString>(CStrID("PhysicsOld"), NULL);
	PhysEntity->Radius = GetEntity()->GetAttr<float>(CStrID("Radius"), 0.3f);
	PhysEntity->Height = GetEntity()->GetAttr<float>(CStrID("Height"), 1.75f);
	PhysEntity->Hover = 0.2f;
	//!!!recreate physics capsule on R/H change!

	EnablePhysics();

	PROP_SUBSCRIBE_PEVENT(AfterPhysics, CPropActorPhysics, AfterPhysics);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropActorPhysics, OnEntityRenamed);
	PROP_SUBSCRIBE_PEVENT(AIBodyRequestLVelocity, CPropActorPhysics, OnRequestLinearVelocity);
	PROP_SUBSCRIBE_PEVENT(AIBodyRequestAVelocity, CPropActorPhysics, OnRequestAngularVelocity);
}
//---------------------------------------------------------------------

void CPropActorPhysics::Deactivate()
{
	UNSUBSCRIBE_EVENT(AfterPhysics);
	UNSUBSCRIBE_EVENT(OnEntityRenamed);
	UNSUBSCRIBE_EVENT(AIBodyRequestLVelocity);
	UNSUBSCRIBE_EVENT(AIBodyRequestAVelocity);

	if (IsEnabled()) DisablePhysics();

	PhysEntity = NULL;

	CPropTransformable::Deactivate();
}
//---------------------------------------------------------------------

void CPropActorPhysics::SetEnabled(bool Enable)
{
	if (Enabled != Enable)
	{
		if (Enable) EnablePhysics();
		else DisablePhysics();
	}
}
//---------------------------------------------------------------------

void CPropActorPhysics::EnablePhysics()
{
	n_assert(!IsEnabled());
	GetEntity()->GetLevel().GetPhysicsOld()->AttachEntity(PhysEntity);
	Stop();
	Enabled = true;
}
//------------------------------------------------------------------------------

void CPropActorPhysics::DisablePhysics()
{
	n_assert(IsEnabled());
	Stop();
	GetEntity()->GetLevel().GetPhysicsOld()->RemoveEntity(PhysEntity);
	Enabled = false;
}
//------------------------------------------------------------------------------

void CPropActorPhysics::GetAABB(bbox3& AABB) const
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

void CPropActorPhysics::Stop()
{
	PhysEntity->SetDesiredLinearVelocity(vector3::Zero);
	GetEntity()->SetAttr<vector4>(CStrID("VelocityVector"), vector4::Zero);
}
//------------------------------------------------------------------------------

void CPropActorPhysics::SetTransform(const matrix44& NewTF)
{
	PhysEntity->SetTransform(NewTF);
	if (!IsEnabled())
	{
		CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
		Physics::CEntity* pPhysEnt = GetPhysicsEntity();
		if (pPhysEnt && pProp && pProp->GetNode())
			pProp->GetNode()->SetWorldTransform(NewTF);

		CPropTransformable::SetTransform(NewTF);
	}
}
//---------------------------------------------------------------------

// The AfterPhysics() method transfers the current physics entity transform to the game entity.
bool CPropActorPhysics::AfterPhysics(const Events::CEventBase& Event)
{
	if (IsEnabled() && PhysEntity->HasTransformChanged())
	{
		CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
		Physics::CEntity* pPhysEnt = GetPhysicsEntity();
		if (pPhysEnt && pProp && pProp->GetNode())
			pProp->GetNode()->SetWorldTransform(PhysEntity->GetTransform());

		CPropTransformable::SetTransform(PhysEntity->GetTransform());

		GetEntity()->SetAttr<vector3>(CStrID("VelocityVector"), PhysEntity->GetVelocity());
	}

	OK;
}
//------------------------------------------------------------------------------

bool CPropActorPhysics::OnEntityRenamed(const Events::CEventBase& Event)
{
	if (PhysEntity.IsValid()) PhysEntity->SetUserData(GetEntity()->GetUID());
	OK;
}
//---------------------------------------------------------------------

bool CPropActorPhysics::OnRequestLinearVelocity(const Events::CEventBase& Event)
{
	const vector4& Velocity = (*((Events::CEvent&)Event).Params).Get<vector4>(CStrID("Velocity"));
	PhysEntity->SetDesiredLinearVelocity(vector3(Velocity.x, Velocity.y, Velocity.z));
	OK;
}
//---------------------------------------------------------------------

bool CPropActorPhysics::OnRequestAngularVelocity(const Events::CEventBase& Event)
{
	PhysEntity->SetDesiredAngularVelocity((*((Events::CEvent&)Event).Params).Get<float>(CStrID("Velocity")));
	OK;
}
//---------------------------------------------------------------------

void CPropActorPhysics::OnRenderDebug()
{
	static const vector4 ColorVel(1.0f, 0.5f, 0.0f, 1.0f);
	static const vector4 ColorDesVel(0.0f, 1.0f, 1.0f, 1.0f);

	const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));

	DebugDraw->DrawCoordAxes(Tfm);
	DebugDraw->DrawLine(Tfm.Translation(), Tfm.Translation() + PhysEntity->GetVelocity(), ColorVel);
	DebugDraw->DrawLine(Tfm.Translation(), Tfm.Translation() + PhysEntity->GetDesiredLinearVelocity(), ColorDesVel);

	if (GetEntity()->GetUID() == CStrID("GG"))
	{
		nString text;
		text.Format("Velocity: %.4f, %.4f, %.4f\nDesired velocity: %.4f, %.4f, %.4f\nAngular desired: %.5f\n"
					"Speed: %.4f\nDesired speed: %.4f",
			PhysEntity->GetVelocity().x,
			PhysEntity->GetVelocity().y,
			PhysEntity->GetVelocity().z,
			PhysEntity->GetDesiredLinearVelocity().x,
			PhysEntity->GetDesiredLinearVelocity().y,
			PhysEntity->GetDesiredLinearVelocity().z,
			PhysEntity->GetDesiredAngularVelocity(),
			PhysEntity->GetVelocity().len(),
			PhysEntity->GetDesiredLinearVelocity().len());
		DebugDraw->DrawText(text.CStr(), 0.5f, 0.0f);
	}
}
//---------------------------------------------------------------------

} // namespace Prop