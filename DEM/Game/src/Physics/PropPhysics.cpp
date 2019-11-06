#include "PropPhysics.h"

#include <Physics/BulletConv.h>
#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Scene/Events/SetTransform.h>
#include <Data/ParamsUtils.h>
#include <Data/DataArray.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <Events/Subscription.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropPhysics, 'PPHY', Game::CProperty);
__ImplementPropertyStorage(CPropPhysics);

bool CPropPhysics::InternalActivate()
{
	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropPhysics, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropPhysics, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(AfterPhysicsTick, CPropPhysics, AfterPhysicsTick);
	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropPhysics, OnSetTransform);
	OK;
}
//---------------------------------------------------------------------

void CPropPhysics::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(AfterPhysicsTick);
	UNSUBSCRIBE_EVENT(SetTransform);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*pProp);
}
//---------------------------------------------------------------------

bool CPropPhysics::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

bool CPropPhysics::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
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

void CPropPhysics::InitSceneNodeModifiers(CPropSceneNode& Prop)
{
	if (!Prop.GetNode()) return;

	Physics::CPhysicsLevel* pPhysWorld = GetEntity()->GetLevel()->GetPhysics();
	if (!pPhysWorld) return;

	const CString& PhysicsDescFile = GetEntity()->GetAttr<CString>(CStrID("Physics"), CString::Empty);    
	if (PhysicsDescFile.IsEmpty()) return;

	Data::PParams PhysicsDesc;
	if (!ParamsUtils::LoadParamsFromPRM(CString("Physics:") + PhysicsDescFile.CStr() + ".prm", PhysicsDesc)) return;
	if (PhysicsDesc.IsNullPtr()) return;

	// Update child nodes' world transform recursively. There are no controllers, so update is finished.
	// It is necessary because dynamic bodies may require subnode world tfm to set their initial tfm.
	// World transform of the prop root node is updated by owner prop.
	Prop.GetNode()->UpdateTransform(nullptr, 0, true, nullptr);

	const Data::CDataArray& Objects = *PhysicsDesc->Get<Data::PDataArray>(CStrID("Objects"));
	for (UPTR i = 0; i < Objects.GetCount(); ++i)
	{
		const Data::CParams& ObjDesc = *Objects.Get<Data::PParams>(i);

		bool IsDynamic = ObjDesc.Get(CStrID("Dynamic"), false);

		Physics::PPhysicsObj Obj;
		if (IsDynamic) Obj = n_new(Physics::CRigidBody);
		else Obj = n_new(Physics::CCollisionObjMoving);
		Obj->SetUserData(*(void**)&GetEntity()->GetUID());

		n_verify_dbg(Obj->Init(ObjDesc, ObjDesc.Get(CStrID("Offset"), vector3::Zero)));

		Scene::CSceneNode* pCurrNode;
		const CString& RelNodePath = ObjDesc.Get<CString>(CStrID("Node"), CString::Empty);
		if (RelNodePath.IsValid())
		{
			//???!!!use node cache?!
			pCurrNode = Prop.GetNode()->GetChild(RelNodePath.CStr());
			n_assert2_dbg(pCurrNode && "Child node not found", RelNodePath.CStr());
		}
		else
		{
			pCurrNode = Prop.GetNode();
			if (RootBody.IsNullPtr() && IsDynamic) RootBody = (Physics::CRigidBody*)Obj.Get();
		}

		if (IsDynamic)
		{
			Obj->SetTransform(pCurrNode->GetWorldMatrix());

			Physics::PNodeControllerRigidBody Ctlr = n_new(Physics::CNodeControllerRigidBody);
			Ctlr->SetBody(*(Physics::CRigidBody*)Obj.Get());
			pCurrNode->SetController(Ctlr);
			Ctlr->Activate(true);

			Ctlrs.Add(Ctlr);
		}
		else
		{
			Physics::PCollisionAttribute Attr = n_new(Physics::CCollisionAttribute);
			Attr->CollObj = (Physics::CCollisionObjMoving*)Obj.Get();
			pCurrNode->AddAttribute(*Attr);
			Attrs.Add(Attr);
		}

		Obj->AttachToLevel(*pPhysWorld);
	}

	if (RootBody.IsValidPtr())
	{
		//!!!angular too!
		vector3 LinVel;
		if (GetEntity()->GetAttr(LinVel, CStrID("LinearVelocity")))
			RootBody->GetBtBody()->setLinearVelocity(VectorToBtVector(LinVel));
	}

	//!!!load joints!
}
//---------------------------------------------------------------------

void CPropPhysics::TermSceneNodeModifiers(CPropSceneNode& Prop)
{
	//!!!unload joints!

	RootBody = nullptr;

	for (UPTR i = 0; i < Ctlrs.GetCount(); ++i)
	{
		Ctlrs[i]->GetBody()->RemoveFromLevel();
		Ctlrs[i]->RemoveFromNode();
	}
	Ctlrs.Clear(); //???create once and attach/detach?

	for (UPTR i = 0; i < Attrs.GetCount(); ++i)
	{
		Attrs[i]->CollObj->RemoveFromLevel();
		Attrs[i]->RemoveFromNode();
	}
	Attrs.Clear(); //???create once and attach/detach?
}
//---------------------------------------------------------------------

bool CPropPhysics::AfterPhysicsTick(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//!!!subscribe only when has meaning!
	//???!!!angular too?!
	if (RootBody.IsNullPtr() || !RootBody->IsInitialized()) FAIL;
	GetEntity()->SetAttr<vector3>(CStrID("LinearVelocity"), BtVectorToVector(RootBody->GetBtBody()->getLinearVelocity()));
	OK;
}
//---------------------------------------------------------------------

bool CPropPhysics::OnSetTransform(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	for (UPTR i = 0; i < Attrs.GetCount(); ++i)
		Attrs[i]->CollObj->SetTransform(Tfm);
	OK;
}
//---------------------------------------------------------------------

}
