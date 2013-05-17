#include "PropAIHints.h"

#include <Game/Entity.h>
#include <Scene/PropSceneNode.h>
#include <Physics/Prop/PropPhysics.h>
#include <AI/AIServer.h>
#include <Render/DebugDraw.h>
#include <Data/DataServer.h>

namespace Prop
{
__ImplementClass(Prop::CPropAIHints, 'PRAH', Game::CProperty);
__ImplementPropertyStorage(CPropAIHints);

static const nString StrStimulusPrefix("AI::CStimulus");

void CPropAIHints::Activate()
{
	Game::CProperty::Activate();

	PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropAIHints, OnPropsActivated);
	PROP_SUBSCRIBE_PEVENT(OnRenderDebug, CPropAIHints, OnRenderDebug);
	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropAIHints, ExposeSI);
	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropAIHints, OnUpdateTransform);
}
//---------------------------------------------------------------------

void CPropAIHints::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(OnRenderDebug);
	UNSUBSCRIBE_EVENT(ExposeSI);
	UNSUBSCRIBE_EVENT(UpdateTransform);

	for (int i = 0; i < Hints.GetCount(); ++i)
	{
		CRecord& Rec = Hints.ValueAtIndex(i);
		if (Rec.pNode) AISrv->GetLevel()->RemoveStimulus(Rec.pNode);
	}

	Hints.Clear();

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropAIHints::OnPropsActivated(const Events::CEventBase& Event)
{
	//???read OnLoad, OnPropsActivated-Deactivate from initialized array?

	//???need to cache?
	Data::PParams Desc;
	const nString& DescName = GetEntity()->GetAttr<nString>(CStrID("AIHintsDesc"), NULL);
	if (DescName.IsValid()) Desc = DataSrv->LoadPRM(nString("aihints:") + DescName + ".prm");

	if (Desc.IsValid())
	{
		const vector3& Pos = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();

		Hints.Clear();
		Hints.BeginAdd();
		for (int i = 0; i < Desc->GetCount(); i++)
		{
			const CParam& Prm = Desc->Get(i);
			PParams PrmVal = Prm.GetValue<PParams>();
			CRecord Rec;
			
			Rec.Stimulus = (CStimulus*)Factory->Create(StrStimulusPrefix + PrmVal->Get<nString>(CStrID("Type"), NULL));

			Rec.Stimulus->SourceEntityID = GetEntity()->GetUID();
			Rec.Stimulus->Position = Pos; //!!!offset * tfm!
			Rec.Stimulus->Intensity = PrmVal->Get<float>(CStrID("Intensity"), 1.f);
			Rec.Stimulus->ExpireTime = PrmVal->Get<float>(CStrID("ExpireTime"), -1.f);

			const nString& SizeStr = PrmVal->Get<nString>(CStrID("Size"), NULL);

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
					bbox3 AABB;
					pNode->GetAABB(AABB);
					vector2 HorizDiag(AABB.vmax.x - AABB.vmin.x, AABB.vmax.z - AABB.vmin.z);
					Rec.Stimulus->Radius = HorizDiag.len() * 0.5f;
					//!!!Rec.Stimulus->Height = AABB.vmax.y - AABB.vmin.y;
				}
			}
			else if (SizeStr == "PhysBox")
			{
				CPropPhysics* pPropPhys = GetEntity()->GetProperty<CPropPhysics>();
				if (pPropPhys)
				{
					bbox3 AABB;
					pPropPhys->GetAABB(AABB);
					vector2 HorizDiag(AABB.vmax.x - AABB.vmin.x, AABB.vmax.z - AABB.vmin.z);
					Rec.Stimulus->Radius = HorizDiag.len() * 0.5f;
					//!!!Rec.Stimulus->Height = AABB.vmax.y - AABB.vmin.y;
				}
			}
			else if (SizeStr == "Attr")
			{
				Rec.Stimulus->Radius = GetEntity()->GetAttr<float>(CStrID("Radius"), 0.3f);
				//!!!Rec.Stimulus->Height = GetEntity()->GetAttr<float>(CStrID("Height"), 1.75f);
			}

			//???Rec.Stimulus->Init(PrmVal);

			Rec.pNode = PrmVal->Get(CStrID("Enabled"), false) ? AISrv->GetLevel()->RegisterStimulus(Rec.Stimulus) : NULL;
			Hints.Add(Prm.GetName(), Rec);
		}
		Hints.EndAdd();
	}

	OK;
}
//---------------------------------------------------------------------

void CPropAIHints::EnableStimulus(CStrID Name, bool Enable)
{
	int Idx = Hints.FindIndex(Name);
	if (Idx != INVALID_INDEX)
	{
		CRecord& Rec = Hints.ValueAtIndex(Idx);
		n_assert(Rec.Stimulus.IsValid());

		if (Enable)
		{
			if (!Rec.pNode)
				Rec.pNode = AISrv->GetLevel()->RegisterStimulus(Rec.Stimulus);
		}
		else
		{
			if (Rec.pNode)
			{
				AISrv->GetLevel()->RemoveStimulus(Rec.pNode);
				Rec.pNode = NULL;
			}
		}
	}
}
//---------------------------------------------------------------------

bool CPropAIHints::OnUpdateTransform(const Events::CEventBase& Event)
{
	const vector3& Pos = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).Translation();
	
	for (int i = 0; i < Hints.GetCount(); ++i)
	{
		CRecord& Rec = Hints.ValueAtIndex(i);
		Rec.Stimulus->Position = Pos; //!!!offset * tfm!
		if (Rec.pNode) AISrv->GetLevel()->UpdateStimulusLocation(Rec.pNode);
	}

	OK;
}
//---------------------------------------------------------------------

bool CPropAIHints::OnRenderDebug(const Events::CEventBase& Event)
{
	static const vector4 ColorVisible(0.7f, 0.0f, 0.0f, 0.5f);
	static const vector4 ColorSound(0.0f, 0.0f, 0.7f, 0.5f);
	static const vector4 ColorOther(0.5f, 0.25f, 0.0f, 0.5f);
	static const vector4 ColorDisabled(0.5f, 0.5f, 0.5f, 0.5f);

	for (int i = 0; i < Hints.GetCount(); ++i)
	{
		CRecord& Rec = Hints.ValueAtIndex(i);

		if (Rec.pNode) //???else draw grey or more pale/transparent sphere?
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

} // namespace Prop
