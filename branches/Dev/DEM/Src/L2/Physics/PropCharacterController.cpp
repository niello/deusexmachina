#include "PropCharacterController.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Scene/Events/SetTransform.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Debug/DebugDraw.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropCharacterController, 'PCCT', Game::CProperty);
__ImplementPropertyStorage(CPropCharacterController);

bool CPropCharacterController::InternalActivate()
{
	CreateController();

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropCharacterController, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropCharacterController, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(RequestLinearV, CPropCharacterController, OnRequestLinearVelocity);
	PROP_SUBSCRIBE_PEVENT(RequestAngularV, CPropCharacterController, OnRequestAngularVelocity);
	PROP_SUBSCRIBE_PEVENT_PRIORITY(BeforePhysicsTick, CPropCharacterController, BeforePhysicsTick, 10); //!!!???REDESIGN!? priority -> explicit call order
	PROP_SUBSCRIBE_PEVENT(AfterPhysicsTick, CPropCharacterController, AfterPhysicsTick);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropCharacterController, OnRenderDebug);
	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropCharacterController, OnSetTransform);
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
	UNSUBSCRIBE_EVENT(AfterPhysicsTick);
	UNSUBSCRIBE_EVENT(OnRenderDebug);
	UNSUBSCRIBE_EVENT(SetTransform);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*pProp);

	CharCtlr = NULL;

}
//---------------------------------------------------------------------

void CPropCharacterController::CreateController()
{
	Physics::CPhysicsWorld* pPhysWorld = GetEntity()->GetLevel()->GetPhysics();
	if (!pPhysWorld) return;

	const CString& PhysicsDescFile = GetEntity()->GetAttr<CString>(CStrID("Physics"), NULL);    
	if (PhysicsDescFile.IsEmpty()) return;

	Data::PParams PhysicsDesc = DataSrv->LoadPRM(CString("Physics:") + PhysicsDescFile.CStr() + ".prm");
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

	if (NodeCtlr.IsValid())
	{
		NodeCtlr->RemoveFromNode();
		NodeCtlr = NULL; //???create once and attach/detach?
	}
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
	CharCtlr->AttachToLevel(*GetEntity()->GetLevel()->GetPhysics());
	pProp->GetNode()->SetController(NodeCtlr);
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
	CharCtlr->RequestLinearVelocity(((Events::CEvent&)Event).Params->Get<vector3>(CStrID("Velocity")));
	OK;
}
//---------------------------------------------------------------------

bool CPropCharacterController::OnRequestAngularVelocity(const Events::CEventBase& Event)
{
	if (!IsEnabled()) FAIL;
	CharCtlr->RequestAngularVelocity(((Events::CEvent&)Event).Params->Get<float>(CStrID("Velocity")));
	OK;
}
//---------------------------------------------------------------------

//???OR USE BULLET ACTION INTERFACE?
bool CPropCharacterController::BeforePhysicsTick(const Events::CEventBase& Event)
{
	if (!IsEnabled()) FAIL; //???or unsubscribe?
	CharCtlr->Update();
	OK;
}
//---------------------------------------------------------------------

bool CPropCharacterController::AfterPhysicsTick(const Events::CEventBase& Event)
{
	if (!IsEnabled()) FAIL; //???or unsubscribe?
	vector3 LinVel;
	if (CharCtlr->GetLinearVelocity(LinVel)) GetEntity()->SetAttr<vector3>(CStrID("LinearVelocity"), LinVel);
	OK;
}
//---------------------------------------------------------------------

bool CPropCharacterController::OnSetTransform(const Events::CEventBase& Event)
{
	const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	CharCtlr->GetBody()->SetTransform(Tfm);
	OK;
}
//---------------------------------------------------------------------

/*
void CPropActorPhysics::GetAABB(CAABB& AABB) const
{
	Physics::CEntity* pEnt = GetPhysicsEntity();
	if (pEnt && pEnt->GetComposite()) pEnt->GetComposite()->GetAABB(AABB);
	else
	{
		AABB.Min.x = 
		AABB.Min.y = 
		AABB.Min.z = 
		AABB.Max.x = 
		AABB.Max.y = 
		AABB.Max.z = 0.f;
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

	if (GetEntity()->GetUID() == "GG" && CharCtlr->IsMotionRequested()) //!!!write debug focus or smth!
	{
		vector3 LVel;
		CharCtlr->GetLinearVelocity(LVel);

		CString Text;
		Text.Format("\n\n\n\n\n\n\n\n"
			"Requested velocity: %.4f, %.4f, %.4f\n"
			"Actual velocity: %.4f, %.4f, %.4f\n"
			"Requested angular velocity: %.5f\n"
			"Actual angular velocity: %.5f\n"
			"Requested speed: %.4f\n"
			"Actual speed: %.4f\n",
			CharCtlr->GetRequestedLinearVelocity().x,
			CharCtlr->GetRequestedLinearVelocity().y,
			CharCtlr->GetRequestedLinearVelocity().z,
			LVel.x,
			LVel.y,
			LVel.z,
			CharCtlr->GetRequestedAngularVelocity(),
			CharCtlr->GetAngularVelocity(),
			CharCtlr->GetRequestedLinearVelocity().Length(),
			LVel.Length());
		DebugDraw->DrawText(Text.CStr(), 0.05f, 0.1f);
	}

	OK;
}
//---------------------------------------------------------------------

}
