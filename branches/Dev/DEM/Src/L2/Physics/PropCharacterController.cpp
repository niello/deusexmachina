#include "PropCharacterController.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Render/DebugDraw.h>

namespace Prop
{
__ImplementClass(Prop::CPropCharacterController, 'PCCT', Game::CProperty);
__ImplementPropertyStorage(CPropCharacterController);

bool CPropCharacterController::InternalActivate()
{
	CreateController();

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*(CPropSceneNode*)pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropCharacterController, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropCharacterController, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(RequestLinearV, CPropCharacterController, OnRequestLinearVelocity);
	PROP_SUBSCRIBE_PEVENT(RequestAngularV, CPropCharacterController, OnRequestAngularVelocity);
	PROP_SUBSCRIBE_PEVENT(BeforePhysicsTick, CPropCharacterController, OnPhysicsTick);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropCharacterController, OnRenderDebug);
	OK;
}
//---------------------------------------------------------------------

void CPropCharacterController::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(RequestLinearV);
	UNSUBSCRIBE_EVENT(RequestAngularV);
	UNSUBSCRIBE_EVENT(BeforePhysicsTick);
	UNSUBSCRIBE_EVENT(OnRenderDebug);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*(CPropSceneNode*)pProp);

	CharCtlr = NULL;

}
//---------------------------------------------------------------------

void CPropCharacterController::CreateController()
{
	Physics::CPhysicsWorld* pPhysWorld = GetEntity()->GetLevel().GetPhysics();
	if (!pPhysWorld) return;

	const nString& PhysicsDescFile = GetEntity()->GetAttr<nString>(CStrID("Physics"), NULL);    
	if (PhysicsDescFile.IsEmpty()) return;

	Data::PParams PhysicsDesc = DataSrv->LoadHRD(nString("physics:") + PhysicsDescFile.CStr() + ".hrd"); //!!!load prm!
	if (!PhysicsDesc.IsValid()) return;

	//???init by entity attrs like R & H instead?
	//???or take them into account?

	CharCtlr = n_new(Physics::CCharacterController);
	CharCtlr->Init(*PhysicsDesc);
	CharCtlr->GetBody()->SetUserData(*(void**)&GetEntity()->GetUID());
}
//---------------------------------------------------------------------

void CPropCharacterController::InitSceneNodeModifiers(CPropSceneNode& Prop)
{
	if (!Prop.GetNode() || !CharCtlr.IsValid()) return;

	NodeCtlr = n_new(Physics::CNodeControllerRigidBody);
	NodeCtlr->SetBody(*CharCtlr->GetBody());

	Enable();
}
//---------------------------------------------------------------------

void CPropCharacterController::TermSceneNodeModifiers(CPropSceneNode& Prop)
{
	if (!Prop.GetNode() || !CharCtlr.IsValid()) return;

	Disable();

	if (Prop.GetNode()->Controller.GetUnsafe() == NodeCtlr)
		Prop.GetNode()->Controller = NULL;
	NodeCtlr = NULL; //???create once and attach/detach?
}
//---------------------------------------------------------------------

bool CPropCharacterController::Enable()
{
	if (!NodeCtlr.IsValid() || !CharCtlr.IsValid()) FAIL;

	if (IsEnabled()) OK;

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pProp || !pProp->IsActive()) FAIL;

	//CharCtlr->GetBody()->SetTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));
	CharCtlr->GetBody()->SetTransform(pProp->GetNode()->GetWorldMatrix());
	CharCtlr->AttachToLevel(*GetEntity()->GetLevel().GetPhysics());
	pProp->GetNode()->Controller = NodeCtlr;
	NodeCtlr->Activate(true);

	OK;
}
//---------------------------------------------------------------------

void CPropCharacterController::Disable()
{
	if (IsEnabled())
	{
		NodeCtlr->Activate(false);
		NodeCtlr->GetBody()->RemoveFromLevel();
	}
}
//---------------------------------------------------------------------

bool CPropCharacterController::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropSceneNode>())
	{
		InitSceneNodeModifiers(*(CPropSceneNode*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropCharacterController::OnPropDeactivating(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropSceneNode>())
	{
		TermSceneNodeModifiers(*(CPropSceneNode*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropCharacterController::OnRequestLinearVelocity(const Events::CEventBase& Event)
{
	if (!IsEnabled()) FAIL;
	const vector3& Velocity = (*((Events::CEvent&)Event).Params).Get<vector4>(CStrID("Velocity"));
	CharCtlr->RequestLinearVelocity(Velocity);
	OK;
}
//---------------------------------------------------------------------

bool CPropCharacterController::OnRequestAngularVelocity(const Events::CEventBase& Event)
{
	if (!IsEnabled()) FAIL;
	CharCtlr->RequestAngularVelocity((*((Events::CEvent&)Event).Params).Get<float>(CStrID("Velocity")));
	OK;
}
//---------------------------------------------------------------------

//???OR USE BULLET ACTION INTERFACE?
bool CPropCharacterController::OnPhysicsTick(const Events::CEventBase& Event)
{
	if (!IsEnabled()) FAIL;
	CharCtlr->Update();
	OK;
}
//---------------------------------------------------------------------

/*
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
*/

bool CPropCharacterController::OnRenderDebug(const Events::CEventBase& Event)
{
	if (!IsEnabled()) OK;

	static const vector4 ColorVel(1.0f, 0.5f, 0.0f, 1.0f);
	static const vector4 ColorReqVel(0.0f, 1.0f, 1.0f, 1.0f);
	static const vector4 ColorCapsuleActive(0.0f, 1.0f, 0.0f, 0.5f);
	static const vector4 ColorCapsuleFrozen(0.0f, 0.0f, 1.0f, 0.5f);

	//const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	matrix44 Tfm;
	vector3 Pos;
	quaternion Rot;
	CharCtlr->GetBody()->GetTransform(Pos, Rot);
	Tfm.FromQuaternion(Rot);
	Tfm.Translation() = Pos;

	DebugDraw->DrawCoordAxes(Tfm);
	vector3 LinVel;
	CharCtlr->GetLinearVelocity(LinVel);
	DebugDraw->DrawLine(Tfm.Translation(), Tfm.Translation() + LinVel, ColorVel);
	DebugDraw->DrawLine(Tfm.Translation(), Tfm.Translation() + CharCtlr->GetRequestedLinearVelocity(), ColorReqVel);

	matrix44 CapsuleTfm = Tfm;
	CapsuleTfm.translate(CharCtlr->GetBody()->GetShapeOffset());
	float CapsuleHeight = CharCtlr->GetHeight() - CharCtlr->GetRadius() - CharCtlr->GetRadius() - CharCtlr->GetHover();
	const vector4& Color = CharCtlr->GetBody()->IsActive() ? ColorCapsuleActive : ColorCapsuleFrozen;
	DebugDraw->DrawCapsule(CapsuleTfm, CharCtlr->GetRadius(), CapsuleHeight, Color);

	/*
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
	*/

	OK;
}
//---------------------------------------------------------------------

}
