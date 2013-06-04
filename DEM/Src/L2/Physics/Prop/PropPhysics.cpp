#include "PropPhysics.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

//!!!OLD!
#include <Physics/PhysicsServerOld.h>
#include <Physics/PhysicsWorldOld.h>
#include <Physics/Event/SetTransform.h>

namespace Prop
{
__ImplementClass(Prop::CPropPhysics, 'PPHY', CPropAbstractPhysics);

using namespace Game;

CPropPhysics::~CPropPhysics()
{
	n_assert(!PhysicsEntity.IsValid());
}
//---------------------------------------------------------------------

void CPropPhysics::Activate()
{
    CPropAbstractPhysics::Activate();

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) InitSceneNodeModifiers(*(CPropSceneNode*)pProp);

	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropPhysics, OnSetTransform);
	PROP_SUBSCRIBE_PEVENT(AfterPhysics, CPropPhysics, AfterPhysics);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropPhysics, OnEntityRenamed);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropPhysics, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropPhysics, OnPropDeactivating);
}
//---------------------------------------------------------------------

void CPropPhysics::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);

	CPropSceneNode* pProp = GetEntity()->GetProperty<CPropSceneNode>();
	if (pProp && pProp->IsActive()) TermSceneNodeModifiers(*(CPropSceneNode*)pProp);

	UNSUBSCRIBE_EVENT(OnEntityRenamed);
	UNSUBSCRIBE_EVENT(AfterPhysics);
	UNSUBSCRIBE_EVENT(SetTransform);
	CPropAbstractPhysics::Deactivate();
    PhysicsEntity = NULL;
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
			Obj->AttachToLevel(*pPhysWorld);

			Physics::PNodeControllerRigidBody Ctlr = n_new(Physics::CNodeControllerRigidBody);
			Ctlr->SetBody(*(Physics::CRigidBody*)Obj.GetUnsafe());
			pCurrNode->Controller = Ctlr;
			Ctlr->Activate(true);

			Ctlrs.Add(pCurrNode, Ctlr);
		}
		else
		{
			Physics::PNodeAttrCollision Attr = n_new(Physics::CNodeAttrCollision);
			pCurrNode->AddAttr(*Attr);
			Attrs.Append(Attr);
		}
	}
}
//---------------------------------------------------------------------

void CPropPhysics::TermSceneNodeModifiers(CPropSceneNode& Prop)
{
	for (int i = 0; i < Ctlrs.GetCount(); ++i)
	{
		Physics::PNodeControllerRigidBody Ctlr = Ctlrs.ValueAtIndex(i);
		if (Ctlrs.KeyAtIndex(i)->Controller.GetUnsafe() == Ctlr)
		{
			//!!!there is a BUG when comment this removal!
			//???can write _working_ autoremoval?
			Ctlr->GetBody()->RemoveFromLevel();
			Ctlrs.KeyAtIndex(i)->Controller = NULL;
		}
	}
	Ctlrs.Clear(); //???create once and attach/detach?

	for (int i = 0; i < Attrs.GetCount(); ++i)
		Attrs[i]->RemoveFromNode();
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

void CPropPhysics::EnablePhysics()
{
	n_assert(!IsEnabled());

	if (!PhysicsEntity.IsValid())
	{
		nString PhysicsDesc;
		if (!GetEntity()->GetAttr<nString>(PhysicsDesc, CStrID("PhysicsOld"))) return;
		PhysicsEntity = CreatePhysicsEntity();
		n_assert(PhysicsEntity.IsValid());
		PhysicsEntity->CompositeName = PhysicsDesc;
		PhysicsEntity->SetUserData(GetEntity()->GetUID());
	}

	// Attach physics entity to physics level
	PhysicsEntity->SetTransform(GetEntity()->GetAttr<matrix44>(CStrID("Transform")));
	GetEntity()->GetLevel().GetPhysicsOld()->AttachEntity(PhysicsEntity);

	PhysicsEntity->SetEnabled(true);

	// Call parent to do the real enable
	CPropAbstractPhysics::EnablePhysics();
}
//---------------------------------------------------------------------

void CPropPhysics::DisablePhysics()
{
	n_assert(IsEnabled());

	// Release the physics entity
	GetEntity()->GetLevel().GetPhysicsOld()->RemoveEntity(GetPhysicsEntity()); // strange design
	CPropAbstractPhysics::DisablePhysics();
}
//---------------------------------------------------------------------

// Called after the physics subsystem has been triggered. This will transfer
// the physics entity's new transform back into the game entity.
bool CPropPhysics::AfterPhysics(const Events::CEventBase& Event)
{
	if (IsEnabled() && PhysicsEntity->HasTransformChanged())
	{
		CPropAbstractPhysics::SetTransform(GetPhysicsEntity()->GetTransform());
		GetEntity()->SetAttr<vector3>(CStrID("VelocityVector"), PhysicsEntity->GetVelocity());
	}
	OK;
}
//---------------------------------------------------------------------

bool CPropPhysics::OnEntityRenamed(const Events::CEventBase& Event)
{
	if (PhysicsEntity.IsValid()) PhysicsEntity->SetUserData(GetEntity()->GetUID());
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
	return Physics::CEntity::CreateInstance();
}
//---------------------------------------------------------------------

} // namespace Prop
