#include "PropPhysics.h"

#include <Physics/BulletConv.h>
#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Scene/Events/SetTransform.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>

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

bool CPropPhysics::OnPropActivated(const Events::CEventBase& Event)
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

bool CPropPhysics::OnPropDeactivating(const Events::CEventBase& Event)
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

	Physics::CPhysicsWorld* pPhysWorld = GetEntity()->GetLevel()->GetPhysics();
	if (!pPhysWorld) return;

	const CString& PhysicsDescFile = GetEntity()->GetAttr<CString>(CStrID("Physics"), NULL);    
	if (PhysicsDescFile.IsEmpty()) return;

	Data::PParams PhysicsDesc = DataSrv->LoadPRM(CString("Physics:") + PhysicsDescFile.CStr() + ".prm");
	if (!PhysicsDesc.IsValid()) return;

	// Update child nodes' world transform recursively. There are no controllers, so update is finished.
	// It is necessary because dynamic bodies may require subnode world tfm to set their initial tfm.
	// World transform of the prop root node is updated by owner prop.
	Prop.GetNode()->UpdateLocalSpace();

	const Data::CDataArray& Objects = *PhysicsDesc->Get<Data::PDataArray>(CStrID("Objects"));
	for (int i = 0; i < Objects.GetCount(); ++i)
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
			if (!RootBody.IsValid() && IsDynamic) RootBody = (Physics::CRigidBody*)Obj.GetUnsafe();
		}

		if (IsDynamic)
		{
			Obj->SetTransform(pCurrNode->GetWorldMatrix());

			Physics::PNodeControllerRigidBody Ctlr = n_new(Physics::CNodeControllerRigidBody);
			Ctlr->SetBody(*(Physics::CRigidBody*)Obj.GetUnsafe());
			pCurrNode->SetController(Ctlr);
			Ctlr->Activate(true);

			Ctlrs.Add(Ctlr);
		}
		else
		{
			Physics::PNodeAttrCollision Attr = n_new(Physics::CNodeAttrCollision);
			Attr->CollObj = (Physics::CCollisionObjMoving*)Obj.GetUnsafe();
			pCurrNode->AddAttr(*Attr);
			Attrs.Add(Attr);
		}

		Obj->AttachToLevel(*pPhysWorld);
	}

	if (RootBody.IsValid())
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

	RootBody = NULL;

	for (int i = 0; i < Ctlrs.GetCount(); ++i)
	{
		Ctlrs[i]->GetBody()->RemoveFromLevel();
		Ctlrs[i]->RemoveFromNode();
	}
	Ctlrs.Clear(); //???create once and attach/detach?

	for (int i = 0; i < Attrs.GetCount(); ++i)
	{
		Attrs[i]->CollObj->RemoveFromLevel();
		Attrs[i]->RemoveFromNode();
	}
	Attrs.Clear(); //???create once and attach/detach?
}
//---------------------------------------------------------------------

bool CPropPhysics::AfterPhysicsTick(const Events::CEventBase& Event)
{
	//!!!subscribe only when has meaning!
	//???!!!angular too?!
	if (!RootBody.IsValid() || !RootBody->IsInitialized()) FAIL;
	GetEntity()->SetAttr<vector3>(CStrID("LinearVelocity"), BtVectorToVector(RootBody->GetBtBody()->getLinearVelocity()));
	OK;
}
//---------------------------------------------------------------------

bool CPropPhysics::OnSetTransform(const Events::CEventBase& Event)
{
	const matrix44& Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	for (int i = 0; i < Attrs.GetCount(); ++i)
		Attrs[i]->CollObj->SetTransform(Tfm);
	OK;
}
//---------------------------------------------------------------------

}
