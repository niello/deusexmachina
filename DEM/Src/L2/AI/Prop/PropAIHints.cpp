#include "PropAIHints.h"

#include <Game/Entity.h>
#include <Physics/Prop/PropPhysics.h>
#include <Loading/EntityFactory.h>
#include <AI/AIServer.h>
#include <Data/DataServer.h>
#include <DB/DBServer.h>

namespace Attr
{
	DeclareAttr(Transform);
	DeclareAttr(Radius);
	//DeclareAttr(Height);

	DefineString(AIHintsDesc);
};

BEGIN_ATTRS_REGISTRATION(PropAIHints)
	RegisterString(AIHintsDesc, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropAIHints, Game::CProperty);
ImplementFactory(Properties::CPropAIHints);
ImplementPropertyStorage(CPropAIHints, 128);
RegisterProperty(CPropAIHints);

static const nString StrStimulusPrefix("AI::CStimulus");

void CPropAIHints::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::AIHintsDesc);
}
//---------------------------------------------------------------------

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

	for (int i = 0; i < Hints.Size(); ++i)
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
	const nString& DescName = GetEntity()->Get<nString>(Attr::AIHintsDesc);
	if (DescName.IsValid()) Desc = DataSrv->LoadPRM(nString("aihints:") + DescName + ".prm");

	if (Desc.isvalid())
	{
		const vector3& Pos = GetEntity()->Get<matrix44>(Attr::Transform).pos_component();

		Hints.Clear();
		Hints.BeginAdd();
		for (int i = 0; i < Desc->GetCount(); i++)
		{
			const CParam& Prm = Desc->Get(i);
			PParams PrmVal = Prm.GetValue<PParams>();
			CRecord Rec;
			
			Rec.Stimulus = (CStimulus*)CoreFct->Create(StrStimulusPrefix + PrmVal->Get<nString>(CStrID("Type"), NULL));

			Rec.Stimulus->SourceEntityID = GetEntity()->GetUniqueID();
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
				//GFX
				//!!!from scene node!

				//CPropGraphics* pPropGfx = GetEntity()->FindProperty<CPropGraphics>();
				//if (pPropGfx)
				//{
				//	bbox3 AABB;
				//	pPropGfx->GetAABB(AABB);
				//	vector2 HorizDiag(AABB.vmax.x - AABB.vmin.x, AABB.vmax.z - AABB.vmin.z);
				//	Rec.Stimulus->Radius = HorizDiag.len() * 0.5f;
				//	//!!!Rec.Stimulus->Height = AABB.vmax.y - AABB.vmin.y;
				//}
			}
			else if (SizeStr == "PhysBox")
			{
				CPropPhysics* pPropPhys = GetEntity()->FindProperty<CPropPhysics>();
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
				Rec.Stimulus->Radius = GetEntity()->Get<float>(Attr::Radius);
				//!!!Rec.Stimulus->Height = GetEntity()->Get<float>(Attr::Height);
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
		n_assert(Rec.Stimulus.isvalid());

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
	const vector3& Pos = GetEntity()->Get<matrix44>(Attr::Transform).pos_component();
	
	for (int i = 0; i < Hints.Size(); ++i)
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

	for (int i = 0; i < Hints.Size(); ++i)
	{
		CRecord& Rec = Hints.ValueAtIndex(i);

		matrix44 Tfm;
		Tfm.scale(vector3(0.2f, 0.2f, 0.2f));
		Tfm.set_translation(Rec.Stimulus->Position);

		//GFX
		/*
		if (Rec.pNode) //???else draw grey or more pale/transparent sphere?
		{
			if (Rec.Stimulus->GetRTTI()->GetName() == "AI::CStimulusVisible")
				nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, Tfm, ColorVisible);
			else if (Rec.Stimulus->GetRTTI()->GetName() == "AI::CStimulusSound")
				nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, Tfm, ColorSound);
			else
				nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, Tfm, ColorOther);
		}
		else nGfxServer2::Instance()->DrawShape(nGfxServer2::Sphere, Tfm, ColorDisabled);
		*/
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
