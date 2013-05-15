#include "PropSmartObject.h"

#include <Game/Entity.h>
#include <AI/AIServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Loading/EntityFactory.h>
#include <DB/DBServer.h>

namespace Attr
{
	DeclareAttr(Transform);

	DefineString(SmartObjDesc);
};

BEGIN_ATTRS_REGISTRATION(PropSmartObject)
	RegisterString(SmartObjDesc, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
__ImplementClassNoFactory(Properties::CPropSmartObject, Game::CProperty);
__ImplementClass(Properties::CPropSmartObject);
__ImplementPropertyStorage(CPropSmartObject, 256);
RegisterProperty(CPropSmartObject);

using namespace Data;

void CPropSmartObject::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	Attrs.Append(Attr::SmartObjDesc);
}
//---------------------------------------------------------------------

void CPropSmartObject::Activate()
{
	Game::CProperty::Activate();

	PParams Desc;
	
	const nString& DescResource = GetEntity()->Get<nString>(Attr::SmartObjDesc);
	if (DescResource.IsValid()) Desc = DataSrv->LoadPRM(nString("smarts:") + DescResource + ".prm");

	if (Desc.IsValid())
	{
		TypeID = CStrID(Desc->Get<nString>(CStrID("TypeID"), NULL).CStr());
		
		PParams DescSection;
		if (Desc->Get<PParams>(DescSection, CStrID("Actions")))
		{
			Actions.BeginAdd(DescSection->GetCount());
			for (int i = 0; i < DescSection->GetCount(); i++)
			{
				const CParam& Prm = DescSection->Get(i);
				PParams ActDesc = Prm.GetValue<PParams>();
				LPCSTR TplName = ActDesc->Get<nString>(CStrID("Tpl")).CStr();
				const CSmartObjActionTpl* pTpl = AISrv->GetSmartObjActionTpl(CStrID(TplName));
				if (pTpl) Actions.Add(Prm.GetName(), n_new(CSmartObjAction)(*pTpl, ActDesc));
				else n_printf("AI, IAO, Warning: can't find smart object action template '%s'\n", TplName);
			}
			Actions.EndAdd();
		}

		nString DefaultState;
		if (Desc->Get(DefaultState, CStrID("DefaultState")))
			SetState(CStrID(DefaultState.CStr()));
	}

	PROP_SUBSCRIBE_PEVENT(ExposeSI, CPropSmartObject, ExposeSI);
}
//---------------------------------------------------------------------

void CPropSmartObject::Deactivate()
{
	UNSUBSCRIBE_EVENT(ExposeSI);

	CurrState = CStrID::Empty;
	Actions.Clear();

	Game::CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropSmartObject::SetState(CStrID ID)
{
	n_assert2(ID.IsValid(), "Tried to set empty SO state");

	//!!!need to know what states are defined for this object!

	PParams P = n_new(CParams(2));
	P->Set(CStrID("From"), CurrState);
	P->Set(CStrID("To"), ID);

	if (CurrState.IsValid()) GetEntity()->FireEvent(CStrID("OnSOStateLeave"), P);
	CurrState = ID;
	GetEntity()->FireEvent(CStrID("OnSOStateEnter"), P);

	OK;
}
//---------------------------------------------------------------------

void CPropSmartObject::EnableAction(CStrID ID, bool Enable)
{
	int Idx = Actions.FindIndex(ID);
	if (Idx != INVALID_INDEX) Actions.ValueAtIndex(Idx)->Enabled = Enable;

	PParams P = n_new(CParams(2));
	P->Set(CStrID("ActionID"), ID);
	P->Set(CStrID("Enabled"), Enable);
	GetEntity()->FireEvent(CStrID("OnSOActionAvailabile"), P);
}
//---------------------------------------------------------------------

bool CPropSmartObject::GetDestination(CStrID ActionID, float ActorRadius, vector3& OutDest, float& OutMinDist, float& OutMaxDist)
{
	PSmartObjAction Action = GetAction(ActionID);

	if (Action.IsValid())
	{
		GetEntity()->Get<matrix44>(Attr::Transform).mult(Action->GetTpl().DestOffset, OutDest);
		OutMinDist = Action->GetTpl().MinDistance;
		OutMaxDist = Action->GetTpl().MaxDistance;
		if (Action->ActorRadiusMatters())
		{
			OutMinDist += ActorRadius;
			OutMaxDist += ActorRadius;
		}
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

} // namespace Properties