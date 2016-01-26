#include "PropAIHints.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Scene/PropSceneNode.h>
#include <Physics/PropPhysics.h>
#include <Scripting/PropScriptable.h>
#include <AI/AIServer.h>
#include <Debug/DebugDraw.h>
#include <Data/DataServer.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropAIHints, 'PRAH', Game::CProperty);
__ImplementPropertyStorage(CPropAIHints);

using namespace Data;

static const CString StrStimulusPrefix("AI::CStimulus");

bool CPropAIHints::InternalActivate()
{
	if (!GetEntity()->GetLevel()->GetAI()) FAIL;

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) EnableSI(*pProp);

	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropAIHints, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropAIHints, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropAIHints, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropAIHints, OnRenderDebug);
	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropAIHints, OnUpdateTransform);
	OK;
}
//---------------------------------------------------------------------

void CPropAIHints::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(OnRenderDebug);
	UNSUBSCRIBE_EVENT(UpdateTransform);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	for (UPTR i = 0; i < Hints.GetCount(); ++i)
	{
		CRecord& Rec = Hints.ValueAt(i);
		if (Rec.QTNode)
		{
			(*Rec.QTNode)->pQTNode->RemoveByHandle(Rec.QTNode);
			(*Rec.QTNode)->pQTNode = NULL;
		}
	}

	Hints.Clear();
}
//---------------------------------------------------------------------

bool CPropAIHints::OnPropActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropAIHints::OnPropDeactivating(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		DisableSI(*(CPropScriptable*)pProp);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropAIHints::OnPropsActivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//???read OnLoad, OnPropsActivated-Deactivate from initialized array?

	//???need to cache?
	Data::PParams Desc;
	const CString& DescName = GetEntity()->GetAttr<CString>(CStrID("AIHintsDesc"), CString::Empty);
	if (DescName.IsValid()) Desc = DataSrv->LoadPRM(CString("AIHints:") + DescName + ".prm");

	if (Desc.IsValidPtr())
	{
		const vector3& Pos = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();

		Hints.Clear();
		Hints.BeginAdd();
		for (UPTR i = 0; i < Desc->GetCount(); ++i)
		{
			const CParam& Prm = Desc->Get(i);
			PParams PrmVal = Prm.GetValue<PParams>();
			CRecord Rec;
			
			Rec.Stimulus = (CStimulus*)Factory->Create(StrStimulusPrefix + PrmVal->Get<CString>(CStrID("Type"), CString::Empty));

			Rec.Stimulus->SourceEntityID = GetEntity()->GetUID();
			Rec.Stimulus->Position = Pos; //!!!offset * tfm!
			Rec.Stimulus->Intensity = PrmVal->Get<float>(CStrID("Intensity"), 1.f);
			Rec.Stimulus->ExpireTime = PrmVal->Get<float>(CStrID("ExpireTime"), -1.f);

			const CString& SizeStr = PrmVal->Get<CString>(CStrID("Size"), CString::Empty);

			if (SizeStr.IsEmpty())
			{
				Rec.Stimulus->Radius = PrmVal->Get<float>(CStrID("Radius"), 0.f);
				//!!!Rec.Stimulus->Height = PrmVal->Get<float>(CStrID("Height"), 0.f);
			}
			else if (SizeStr == "Box" || SizeStr == "GfxBox")
			{
				CPropSceneNode* pNode = GetEntity()->GetProperty<CPropSceneNode>();
				if (pNode)
				{
					CAABB AABB;
					pNode->GetAABB(AABB);
					vector2 HorizDiag(AABB.Max.x - AABB.Min.x, AABB.Max.z - AABB.Min.z);
					Rec.Stimulus->Radius = HorizDiag.Length() * 0.5f;
					//!!!Rec.Stimulus->Height = AABB.Max.y - AABB.Min.y;
				}
			}
			else if (SizeStr == "PhysBox")
			{
				CPropPhysics* pPropPhys = GetEntity()->GetProperty<CPropPhysics>();
				if (pPropPhys)
				{
					CAABB AABB;
					pPropPhys->GetAABB(AABB);
					vector2 HorizDiag(AABB.Max.x - AABB.Min.x, AABB.Max.z - AABB.Min.z);
					Rec.Stimulus->Radius = HorizDiag.Length() * 0.5f;
					//!!!Rec.Stimulus->Height = AABB.Max.y - AABB.Min.y;
				}
			}
			else if (SizeStr == "Attr")
			{
				Rec.Stimulus->Radius = GetEntity()->GetAttr<float>(CStrID("Radius"), 0.3f);
				//!!!Rec.Stimulus->Height = GetEntity()->GetAttr<float>(CStrID("Height"), 1.75f);
			}

			//???Rec.Stimulus->Init(PrmVal);

			Rec.QTNode = PrmVal->Get(CStrID("Enabled"), false) ? GetEntity()->GetLevel()->GetAI()->RegisterStimulus(Rec.Stimulus) : NULL;
			Hints.Add(Prm.GetName(), Rec);
		}
		Hints.EndAdd();
	}

	OK;
}
//---------------------------------------------------------------------

void CPropAIHints::EnableStimulus(CStrID Name, bool Enable)
{
	IPTR Idx = Hints.FindIndex(Name);
	if (Idx != INVALID_INDEX)
	{
		CRecord& Rec = Hints.ValueAt(Idx);
		n_assert(Rec.Stimulus.IsValidPtr());

		if (Enable)
		{
			if (!Rec.QTNode)
				Rec.QTNode = GetEntity()->GetLevel()->GetAI()->RegisterStimulus(Rec.Stimulus);
		}
		else
		{
			if (Rec.QTNode)
			{
				(*Rec.QTNode)->pQTNode->RemoveByHandle(Rec.QTNode);
				(*Rec.QTNode)->pQTNode = NULL;
				Rec.QTNode = NULL;
			}
		}
	}
}
//---------------------------------------------------------------------

bool CPropAIHints::OnUpdateTransform(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const vector3& Pos = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();
	
	for (UPTR i = 0; i < Hints.GetCount(); ++i)
	{
		CRecord& Rec = Hints.ValueAt(i);
		Rec.Stimulus->Position = Pos; //!!!offset * tfm!
		if (Rec.QTNode) GetEntity()->GetLevel()->GetAI()->UpdateStimulusLocation(Rec.QTNode);
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropAIHints::OnRenderDebug(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	static const vector4 ColorVisible(0.7f, 0.0f, 0.0f, 0.5f);
	static const vector4 ColorSound(0.0f, 0.0f, 0.7f, 0.5f);
	static const vector4 ColorOther(0.5f, 0.25f, 0.0f, 0.5f);
	static const vector4 ColorDisabled(0.5f, 0.5f, 0.5f, 0.5f);

	for (UPTR i = 0; i < Hints.GetCount(); ++i)
	{
		CRecord& Rec = Hints.ValueAt(i);

		if (Rec.QTNode) //???else draw grey or more pale/transparent sphere?
		{
			if (Rec.Stimulus->GetRTTI()->GetName() == "AI::CStimulusVisible")
				DebugDraw->DrawSphere(Rec.Stimulus->Position, 0.2f, ColorVisible);
			else if (Rec.Stimulus->GetRTTI()->GetName() == "AI::CStimulusSound")
				DebugDraw->DrawSphere(Rec.Stimulus->Position, 0.2f, ColorSound);
			else
				DebugDraw->DrawSphere(Rec.Stimulus->Position, 0.2f, ColorOther);
		}
		else DebugDraw->DrawSphere(Rec.Stimulus->Position, 0.2f, ColorDisabled);
	}

	OK;
}
//---------------------------------------------------------------------

}