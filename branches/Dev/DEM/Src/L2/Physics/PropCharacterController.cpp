#include "PropCharacterController.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Prop
{
__ImplementClass(Prop::CPropCharacterController, 'PCCT', Game::CProperty);
__ImplementPropertyStorage(CPropCharacterController);

void CPropCharacterController::Activate()
{
    CProperty::Activate();

	CreateController();

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*(CPropSceneNode*)pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropCharacterController, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropCharacterController, OnPropDeactivating);
}
//---------------------------------------------------------------------

void CPropCharacterController::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*(CPropSceneNode*)pProp);

	CharCtlr = NULL;

	CProperty::Deactivate();
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

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (!pProp || !pProp->IsActive()) FAIL;

	if (!IsEnabled())
	{
		CharCtlr->GetBody()->SetTransform(pProp->GetNode()->GetWorldMatrix());
		CharCtlr->AttachToLevel(*GetEntity()->GetLevel().GetPhysics());
		pProp->GetNode()->Controller = NodeCtlr;
		NodeCtlr->Activate(true);
	}

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

}
