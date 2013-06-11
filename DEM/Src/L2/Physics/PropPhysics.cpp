#include "PropPhysics.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Prop
{
__ImplementClass(Prop::CPropPhysics, 'PPHY', Game::CProperty);
__ImplementPropertyStorage(CPropPhysics);

bool CPropPhysics::InternalActivate()
{
	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*(CPropSceneNode*)pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropPhysics, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropPhysics, OnPropDeactivating);
	OK;
}
//---------------------------------------------------------------------

void CPropPhysics::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*(CPropSceneNode*)pProp);
}
//---------------------------------------------------------------------

void CPropPhysics::InitSceneNodeModifiers(CPropSceneNode& Prop)
{
	if (!Prop.GetNode()) return;

	Physics::CPhysicsWorld* pPhysWorld = GetEntity()->GetLevel().GetPhysics();
	if (!pPhysWorld) return;

	const nString& PhysicsDescFile = GetEntity()->GetAttr<nString>(CStrID("Physics"), NULL);    
	if (PhysicsDescFile.IsEmpty()) return;

	Data::PParams PhysicsDesc = DataSrv->LoadHRD(nString("physics:") + PhysicsDescFile.CStr() + ".hrd"); //!!!load prm!
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

		n_verify_dbg(Obj->Init(ObjDesc)); //???where to get offset?

		Scene::CSceneNode* pCurrNode;
		const nString& RelNodePath = ObjDesc.Get<nString>(CStrID("Node"), nString::Empty);
		if (RelNodePath.IsValid())
		{
			//???!!!use node cache?!
			pCurrNode = Prop.GetNode()->GetChild(RelNodePath.CStr());
			n_assert2_dbg(pCurrNode && "Child node not found", RelNodePath.CStr());
		}
		else pCurrNode = Prop.GetNode();

		if (IsDynamic)
		{
			Obj->SetTransform(pCurrNode->GetWorldMatrix());

			Physics::PNodeControllerRigidBody Ctlr = n_new(Physics::CNodeControllerRigidBody);
			Ctlr->SetBody(*(Physics::CRigidBody*)Obj.GetUnsafe());
			pCurrNode->Controller = Ctlr;
			Ctlr->Activate(true);

			Ctlrs.Add(pCurrNode, Ctlr);
		}
		else
		{
			Physics::PNodeAttrCollision Attr = n_new(Physics::CNodeAttrCollision);
			Attr->CollObj = (Physics::CCollisionObjMoving*)Obj.GetUnsafe();
			pCurrNode->AddAttr(*Attr);
			Attrs.Append(Attr);
		}

		Obj->AttachToLevel(*pPhysWorld);
	}

	//!!!load joints!
}
//---------------------------------------------------------------------

void CPropPhysics::TermSceneNodeModifiers(CPropSceneNode& Prop)
{
	//!!!unload joints!

	for (int i = 0; i < Ctlrs.GetCount(); ++i)
	{
		Physics::PNodeControllerRigidBody Ctlr = Ctlrs.ValueAtIndex(i);
		Ctlr->GetBody()->RemoveFromLevel();
		if (Ctlrs.KeyAtIndex(i)->Controller.GetUnsafe() == Ctlr)
			Ctlrs.KeyAtIndex(i)->Controller = NULL;
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

}
