#include "PropSmartObject.h"

#include <Game/Entity.h>
#include <Scripting/PropScriptable.h>
#include <AI/AIServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Prop
{
__ImplementClass(Prop::CPropSmartObject, 'PRSO', Game::CProperty);
__ImplementPropertyStorage(CPropSmartObject);

bool CPropSmartObject::InternalActivate()
{
	CurrState = GetEntity()->GetAttr(CStrID("SOState"), CStrID::Empty);
	GetEntity()->SetAttr(CStrID("SOState"), CurrState); // Make sure the attribute is set

	Data::PParams Desc;
	const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), NULL);
	if (DescResource.IsValid()) Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");

	if (Desc.IsValid())
	{
		TypeID = Desc->Get(CStrID("TypeID"), CStrID::Empty);
		Movable = Desc->Get(CStrID("Movable"), false);

		Data::PParams ActionsData, ActionsEnabled, ActionsProgress;
		if (GetEntity()->GetAttr(ActionsData, CStrID("SOActions")))
		{
			ActionsData->Get(ActionsEnabled, CStrID("Enabled"));
			ActionsData->Get(ActionsProgress, CStrID("Progress"));
		}

		Data::PParams DescSection;
		if (Desc->Get<Data::PParams>(DescSection, CStrID("Actions")))
		{
			Actions.BeginAdd(DescSection->GetCount());
			for (int i = 0; i < DescSection->GetCount(); i++)
			{
				const Data::CParam& Prm = DescSection->Get(i);
				Data::PParams ActDesc = Prm.GetValue<Data::PParams>();
				LPCSTR TplName = ActDesc->Get<CString>(CStrID("Tpl")).CStr();
				const AI::CSmartObjActionTpl* pTpl = AISrv->GetSmartObjActionTpl(CStrID(TplName));
				if (pTpl)
				{
					AI::PSmartObjAction Action = n_new(AI::CSmartObjAction)(*pTpl, ActDesc);
					if (!CurrState.IsValid()) Action->Enabled = false;
					else if (ActionsEnabled.IsValid()) ActionsEnabled->Get(Action->Enabled, Prm.GetName());
					if (ActionsProgress.IsValid()) ActionsProgress->Get(Action->Progress, Prm.GetName());
					Actions.Add(Prm.GetName(), Action);
				}
				else n_printf("AI, SO, Warning: can't find smart object action template '%s'\n", TplName);
			}
			Actions.EndAdd();
		}
	}

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive())
	{
		EnableSI(*pProp);
		GetEntity()->FireEvent(CStrID("OnSOLoaded")); //???or in OnPropsActivated?
	}

	if (!CurrState.IsValid() && Desc.IsValid() && Desc->Has(CStrID("DefaultState")))
	{
		// Delayed default state setting, see comment to an OnPropsActivated()
		PROP_SUBSCRIBE_PEVENT(OnPropsActivated, CPropSmartObject, OnPropsActivated);
	}
	PROP_SUBSCRIBE_PEVENT(OnPropActivated, CPropSmartObject, OnPropActivated);
	PROP_SUBSCRIBE_PEVENT(OnPropDeactivating, CPropSmartObject, OnPropDeactivating);
	PROP_SUBSCRIBE_PEVENT(OnLevelSaving, CPropSmartObject, OnLevelSaving);
	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(OnPropsActivated);
	UNSUBSCRIBE_EVENT(OnPropActivated);
	UNSUBSCRIBE_EVENT(OnPropDeactivating);
	UNSUBSCRIBE_EVENT(OnLevelSaving);

	CPropScriptable* pProp = GetEntity()->GetProperty<CPropScriptable>();
	if (pProp && pProp->IsActive()) DisableSI(*pProp);

	CurrState = CStrID::Empty;
	Actions.Clear();
}
//---------------------------------------------------------------------

// Empty state has a non-conditional transition to DefaultState from the Desc, if it is defined.
// Default state must be set through SetState, and script feedback is desired. We delay state
// setting to make sure that a possible PropScriptable is activated. If you want to use empty
// state as 'Disabled', create explicit 'Disabled' state without this hardcoded transition.
// NB: all checks were already performed on subscription.
bool CPropSmartObject::OnPropsActivated(const Events::CEventBase& Event)
{
	const CString& DescResource = GetEntity()->GetAttr<CString>(CStrID("SODesc"), NULL);
	Data::PParams Desc = DataSrv->LoadPRM(CString("Smarts:") + DescResource + ".prm");
	SetState(Desc->Get<CStrID>(CStrID("DefaultState")));
	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropActivated(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Game::CProperty* pProp = (Game::CProperty*)P->Get<PVOID>(CStrID("Prop"));
	if (!pProp) FAIL;

	if (pProp->IsA<CPropScriptable>())
	{
		EnableSI(*(CPropScriptable*)pProp);
		GetEntity()->FireEvent(CStrID("OnSOLoaded")); //???or in OnPropsActivated?
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CPropSmartObject::OnPropDeactivating(const Events::CEventBase& Event)
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

bool CPropSmartObject::OnLevelSaving(const Events::CEventBase& Event)
{
	// Need to recreate params because else we may rewrite initial level desc in the HRD cache
	Data::PParams P = n_new(Data::CParams(2));
	Data::PParams PEnabled = n_new(Data::CParams(Actions.GetCount()));
	Data::PParams PProgress = n_new(Data::CParams(Actions.GetCount()));

	for (int i = 0; i < Actions.GetCount(); ++i)
	{
		AI::CSmartObjAction& Action = *Actions.ValueAt(i);
		PEnabled->Set(Actions.KeyAt(i), Action.Enabled);
		if (Action.Progress > 0.f) PProgress->Set(Actions.KeyAt(i), Action.Progress);
	}

	P->Set(CStrID("Enabled"), PEnabled);
	if (PProgress->GetCount()) P->Set(CStrID("Progress"), PProgress);
	GetEntity()->SetAttr<Data::PParams>(CStrID("SOActions"), P);

	OK;
}
//---------------------------------------------------------------------

bool CPropSmartObject::SetState(CStrID ID)
{
	n_assert2(ID.IsValid(), "CPropSmartObject::SetState() > Tried to set empty state");

	//!!!need to know what states are defined for this object!

	if (ID == CurrState) OK;

	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("From"), CurrState);
	P->Set(CStrID("To"), ID);

	if (CurrState.IsValid()) GetEntity()->FireEvent(CStrID("OnSOStateLeave"), P);
	CurrState = ID;
	GetEntity()->SetAttr(CStrID("SOState"), CurrState);
	GetEntity()->FireEvent(CStrID("OnSOStateEnter"), P);

	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::EnableAction(CStrID ID, bool Enable)
{
	int Idx = Actions.FindIndex(ID);
	if (Idx == INVALID_INDEX) return;

	AI::CSmartObjAction* pAct = Actions.ValueAt(Idx).GetUnsafe();
	if (pAct->Enabled == Enable) return;
	pAct->Enabled = Enable;

	Data::PParams P = n_new(Data::CParams(2));
	P->Set(CStrID("ActionID"), ID);
	P->Set(CStrID("Enabled"), Enable);
	GetEntity()->FireEvent(CStrID("OnSOActionAvailabile"), P);
}
//---------------------------------------------------------------------

bool CPropSmartObject::GetDestinationParams(CStrID ActionID, float ActorRadius, vector3& OutOffset, float& OutMinDist, float& OutMaxDist)
{
	AI::PSmartObjAction Action = GetAction(ActionID);

	if (Action.IsValid())
	{
		matrix33 Tfm = GetEntity()->GetAttr<matrix44>(CStrID("Transform")).ToMatrix33();
		Tfm.mult(Action->GetTpl().DestOffset, OutOffset);
		OutMinDist = Action->GetTpl().MinDistance;
		OutMaxDist = Action->GetTpl().MaxDistance;
		if (Action->ActorRadiusMatters())
		{
			OutMinDist += ActorRadius;
			OutMaxDist += ActorRadius;
		}
		//???add SORadiusMatters? for items, enemies etc
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

}