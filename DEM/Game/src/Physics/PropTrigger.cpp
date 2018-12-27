#include "PropTrigger.h"

#include <Game/Entity.h>
#include <Game/GameServer.h>
#include <Game/GameLevel.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/TriggerContactCallback.h>
#include <Scripting/PropScriptable.h>
#include <Events/Subscription.h>
#include <Debug/DebugDraw.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropTrigger, 'PTRG', Game::CProperty);
__ImplementPropertyStorage(CPropTrigger);

bool CPropTrigger::InternalActivate()
{
	Period = GetEntity()->GetAttr<float>(CStrID("TrgPeriod"));
	if (Period > 0.f) TimeLastTriggered = GetEntity()->GetAttr<float>(CStrID("TrgTimeLastTriggered"));

	//!!!???get trigger shape, offset etc from physics desc?! can also fine-tune coll mask in desc!
	Physics::PCollisionShape Shape;
	const vector4& ShapeParams = GetEntity()->GetAttr<vector4>(CStrID("TrgShapeParams"), vector4::White);
	switch (GetEntity()->GetAttr<int>(CStrID("TrgShapeType")))
	{
		case 1: Shape = PhysicsSrv->CreateBoxShape(ShapeParams); break;
		case 2: Shape = PhysicsSrv->CreateSphereShape(ShapeParams.x); break;
		case 4: Shape = PhysicsSrv->CreateCapsuleShape(ShapeParams.x, ShapeParams.y); break;
		default: Sys::Error("Entity '%s': CPropTrigger::Activate(): Shape type %d unsupported\n",
					 GetEntity()->GetUID().CStr(),
					 GetEntity()->GetAttr<int>(CStrID("TrgShapeType")));
	}

	CollObj = n_new(Physics::CCollisionObjStatic);
	U16 Group = PhysicsSrv->CollisionGroups.GetMask("Trigger");
	U16 Mask = PhysicsSrv->CollisionGroups.GetMask("All") & ~Group;
	CollObj->Init(*Shape, Group, Mask); // Can specify offset
	CollObj->GetBtObject()->setCollisionFlags(CollObj->GetBtObject()->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

	SetEnabled(GetEntity()->GetAttr<bool>(CStrID("TrgEnabled")));

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) EnableSI(*pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropTrigger, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropTrigger, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropTrigger, OnLevelSaving);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropTrigger, OnRenderDebug);
	OK;
}
//---------------------------------------------------------------------

void CPropTrigger::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnLevelSaving);
	UNSUBSCRIBE_EVENT(OnRenderDebug);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	SetEnabled(false);

	CollObj = NULL;
	pScriptObj = NULL;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		CPropScriptable* pScriptable = GetEntity()->GetProperty<CPropScriptable>();
		pScriptObj = pScriptable ? pScriptable->GetScriptObject() : NULL;
		EnableSI(*pScriptable);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		pScriptObj = NULL;
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

void CPropTrigger::SetEnabled(bool Enable)
{
	if (Enabled == Enable) return;
	if (Enable) PROP_SUBSCRIBE_PEVENT(OnBeginFrame, CPropTrigger, OnBeginFrame);
	else
	{
		if (pScriptObj)
			for (UPTR i = 0; i < CurrInsiders.GetCount(); ++i)
				pScriptObj->RunFunctionOneArg("OnTriggerLeave", CString(CurrInsiders[i].CStr()));
		CurrInsiders.Clear();

		UNSUBSCRIBE_EVENT(OnBeginFrame);
	}
	Enabled = Enable;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnBeginFrame(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	CArray<const btCollisionObject*> Collisions(16, 16);
	Physics::CTriggerContactCallback TriggerCB(CollObj->GetBtObject(), Collisions, CollObj->GetCollisionGroup(), CollObj->GetCollisionMask());
	GetEntity()->GetLevel()->GetPhysics()->GetBtWorld()->contactTest(CollObj->GetBtObject(), TriggerCB);

	CArray<CStrID> NewInsiders(16, 16);

	// Sort to skip duplicates
	Collisions.Sort();
	const btCollisionObject* pCurrObj = NULL;
	for (UPTR i = 0; i < Collisions.GetCount(); ++i)
	{
		if (Collisions[i] == pCurrObj) continue;
		pCurrObj = Collisions[i];
		if (!pCurrObj || !pCurrObj->getUserPointer()) continue;
		Physics::PPhysicsObj PhysObj = (Physics::CPhysicsObject*)pCurrObj->getUserPointer();
		void* pUserData = PhysObj->GetUserData();
		if (!pUserData) continue;
		CStrID EntityID = *(CStrID*)&pUserData;
		if (pScriptObj && CurrInsiders.FindIndexSorted(EntityID) == INVALID_INDEX)
		{
			Data::CData RetVal;
			pScriptObj->RunFunctionOneArg("OnTriggerEnter", CString(EntityID.CStr()), &RetVal);
			if (!(RetVal.IsA<bool>() && RetVal == false || RetVal.IsA<int>() && RetVal == 0))
				NewInsiders.Add(EntityID);
		}
	}

	NewInsiders.Sort();

	if (pScriptObj)
		for (UPTR i = 0; i < CurrInsiders.GetCount(); ++i)
		{
			CStrID EntityID = CurrInsiders[i];
			if (NewInsiders.FindIndexSorted(EntityID) == INVALID_INDEX && GameSrv->GetEntityMgr()->EntityExists(EntityID))
				pScriptObj->RunFunctionOneArg("OnTriggerLeave", CString(EntityID.CStr()));
		}

	CurrInsiders = NewInsiders;

	float NewTime = (float)GameSrv->GetTime();
	if (Period > 0.f && NewTime - TimeLastTriggered >= Period)
	{
		if (pScriptObj)
			for (UPTR i = 0; i < CurrInsiders.GetCount(); ++i)
				pScriptObj->RunFunctionOneArg("OnTriggerApply", CString(CurrInsiders[i].CStr()));
		TimeLastTriggered = NewTime;
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnLevelSaving(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	GetEntity()->SetAttr<bool>(CStrID("TrgEnabled"), Enabled);
	if (Period > 0.f) GetEntity()->SetAttr<float>(CStrID("TrgTimeLastTriggered"), TimeLastTriggered);
	OK;
}
//---------------------------------------------------------------------

bool CPropTrigger::OnRenderDebug(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	static const vector4 ColorOn(0.9f, 0.58f, 1.0f, 0.3f); // purple
	static const vector4 ColorOff(0.0f, 0.0f, 0.0f, 0.08f); // black

	matrix44 EntityTfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform"));
	EntityTfm.translate(CollObj->GetShapeOffset());

	const vector4& ShapeParams = GetEntity()->GetAttr<vector4>(CStrID("TrgShapeParams"));
	switch (GetEntity()->GetAttr<int>(CStrID("TrgShapeType")))
	{
		case 1:
		{
			matrix44 Tfm(
				ShapeParams.x, 0.f, 0.f, 0.f,
				0.f, ShapeParams.y, 0.f, 0.f,
				0.f, 0.f, ShapeParams.z, 0.f,
				0.f, 0.f, 0.f, 1.f);
			DebugDraw->DrawBox(Tfm * EntityTfm, Enabled ? ColorOn : ColorOff);
			break;
		}
		case 2:
			DebugDraw->DrawSphere(EntityTfm.Translation(), ShapeParams.x, Enabled ? ColorOn : ColorOff);
			break;
		case 4:
			DebugDraw->DrawCapsule(EntityTfm, ShapeParams.x, ShapeParams.y, Enabled ? ColorOn : ColorOff);
			break;
		default: break;
	}

	OK;
}
//---------------------------------------------------------------------

}