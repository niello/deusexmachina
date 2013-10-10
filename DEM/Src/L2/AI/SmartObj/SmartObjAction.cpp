#include "SmartObjAction.h"

#include <Data/Params.h>

namespace AI
{

	CSmartObjAction::CSmartObjAction(const CSmartObjActionTpl& _Tpl, Data::PParams Desc):
	Tpl(_Tpl), FreeUserSlots(_Tpl.MaxUserCount), Progress(0.f)
{
	if (Desc.IsValid())
	{
		Enabled = Desc->Get<bool>(CStrID("Enabled"), true);
		VisibleInUI = Desc->Get<bool>(CStrID("VisibleInUI"), true);
		Resource = Desc->Get<int>(CStrID("Resource"), -1);

		static const CString StrValidatorPfx("AI::CValidator");
		static const CString StrWSSrcPfx("AI::CWorldStateSource");

		Data::PParams SubDesc = Desc->Get<Data::PParams>(CStrID("Validator"), NULL);
		if (SubDesc.IsValid())
		{
			ActivationValidator = (CValidator*)Factory->Create(StrValidatorPfx + SubDesc->Get<CString>(CStrID("Type")));
			ActivationValidator->Init(SubDesc);
			UpdateValidator = ActivationValidator;
		}
		else
		{
			SubDesc = Desc->Get<Data::PParams>(CStrID("ActivationValidator"), NULL);
			if (SubDesc.IsValid())
			{
				ActivationValidator = (CValidator*)Factory->Create(StrValidatorPfx + SubDesc->Get<CString>(CStrID("Type")));
				ActivationValidator->Init(SubDesc);
			}

			SubDesc = Desc->Get<Data::PParams>(CStrID("UpdateValidator"), NULL);
			if (SubDesc.IsValid())
			{
				UpdateValidator = (CValidator*)Factory->Create(StrValidatorPfx + SubDesc->Get<CString>(CStrID("Type")));
				UpdateValidator->Init(SubDesc);
			}
		}

		OnStartCmd = CStrID(Desc->Get<CString>(CStrID("OnStartCmd"), CString::Empty).CStr());
		OnDoneCmd = CStrID(Desc->Get<CString>(CStrID("OnDoneCmd"), CString::Empty).CStr());
		OnEndCmd = CStrID(Desc->Get<CString>(CStrID("OnEndCmd"), CString::Empty).CStr());
		OnAbortCmd = CStrID(Desc->Get<CString>(CStrID("OnAbortCmd"), CString::Empty).CStr());

		SubDesc = Desc->Get<Data::PParams>(CStrID("Preconditions"), NULL);
		if (SubDesc.IsValid())
		{
			Preconditions = (CWorldStateSource*)Factory->Create(StrWSSrcPfx + SubDesc->Get<CString>(CStrID("Type")));
			Preconditions->Init(SubDesc);
		}
	}
	else
	{
		Enabled = true;
		VisibleInUI = true;
		Resource = -1;
		ActivationValidator = NULL;
		UpdateValidator = NULL;
		OnStartCmd =
		OnDoneCmd =
		OnEndCmd =
		OnAbortCmd = CStrID::Empty;
		Preconditions = NULL;
	}
}
//---------------------------------------------------------------------

} //namespace AI