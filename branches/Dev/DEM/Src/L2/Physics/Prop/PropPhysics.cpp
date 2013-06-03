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

//BEGIN_ATTRS_REGISTRATION(PropPhysics)
//	RegisterString(Physics, ReadOnly);
//	RegisterVector3(VelocityVector, ReadOnly);
//END_ATTRS_REGISTRATION

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
	if (pProp && pProp->IsActive()) SetupScene(*(CPropSceneNode*)pProp);

	PROP_SUBSCRIBE_NEVENT(SetTransform, CPropPhysics, OnSetTransform);
	PROP_SUBSCRIBE_PEVENT(AfterPhysics, CPropPhysics, AfterPhysics);
	PROP_SUBSCRIBE_PEVENT(OnEntityRenamed, CPropPhysics, OnEntityRenamed);
}
//---------------------------------------------------------------------

void CPropPhysics::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnEntityRenamed);
	UNSUBSCRIBE_EVENT(AfterPhysics);
	UNSUBSCRIBE_EVENT(SetTransform);
	CPropAbstractPhysics::Deactivate();
    PhysicsEntity = NULL;
}
//---------------------------------------------------------------------

void CPropPhysics::SetupScene(CPropSceneNode& Prop)
{
	const nString& PhysicsDescFile = GetEntity()->GetAttr<nString>(CStrID("Physics"), NULL);    
	if (PhysicsDescFile.IsValid() && GetEntity()->GetLevel().GetPhysics())
	{
		Data::PParams PhysicsDesc = DataSrv->LoadHRD(nString("physics:") + PhysicsDescFile.CStr() + ".hrd"); //!!!load prm!
		if (PhysicsDesc.IsValid())
		{
			const Data::CDataArray& Objects = *PhysicsDesc->Get<Data::PDataArray>(CStrID("Objects"));
			for (int i = 0; i < Objects.GetCount(); ++i)
			{
				//???allow moving collision objects and rigid bodies?

				//const Data::CParams& ObjDesc = *Objects.Get<Data::PParams>(i);
				//CollObj = n_new(Physics::CCollisionObjStatic);
				//CollObj->Init(ObjDesc); //???where to get offset?

				//Scene::CSceneNode* pCurrNode = Node.GetUnsafe();
				//const nString& RelNodePath = ObjDesc.Get<nString>(CStrID("Node"), nString::Empty);
				//if (pCurrNode && RelNodePath.IsValid())
				//{
				//	pCurrNode = pCurrNode->GetChild(RelNodePath.CStr());
				//	n_assert2_dbg(pCurrNode && "Child node not found", RelNodePath.CStr());
				//}

				//CollObj->SetTransform(pCurrNode ? pCurrNode->GetWorldMatrix() : EntityTfm);
				//CollObj->AttachToLevel(*Level->GetPhysics());
			}
		}
	}
}
//---------------------------------------------------------------------

bool CPropPhysics::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropSceneNode>())
	{
		SetupScene(*(CPropSceneNode*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropPhysics::OnPropDeactivating(const Events::CEventBase& Event)
{
	OK;
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
