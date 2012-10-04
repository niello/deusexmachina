#include "SmartObjAction.h"

#include <Data/Params.h>

namespace AI
{
using namespace Data;

CSmartObjAction::CSmartObjAction(const CSmartObjActionTpl& _Tpl, PParams Desc):
	Tpl(_Tpl), FreeUserSlots(_Tpl.MaxUserCount), Progress(0.f)
{
	if (Desc.isvalid())
	{
		Enabled = Desc->Get<bool>(CStrID("Enabled"), true);

		AppearsInUI = Desc->Get<bool>(CStrID("AppearsInUI"), true);
		Resource = Desc->Get<int>(CStrID("Resource"), -1);

		static const nString StrValidatorPfx("AI::CValidator");
		static const nString StrWSSrcPfx("AI::CWorldStateSource");

		PParams SubDesc = Desc->Get<PParams>(CStrID("Validator"), NULL);
		if (SubDesc.isvalid())
		{
			ActivationValidator = (CValidator*)CoreFct->Create(StrValidatorPfx + SubDesc->Get<nString>(CStrID("Type")));
			ActivationValidator->Init(SubDesc);
			UpdateValidator = ActivationValidator;
		}
		else
		{
			SubDesc = Desc->Get<PParams>(CStrID("ActivationValidator"), NULL);
			if (SubDesc.isvalid())
			{
				ActivationValidator = (CValidator*)CoreFct->Create(StrValidatorPfx + SubDesc->Get<nString>(CStrID("Type")));
				ActivationValidator->Init(SubDesc);
			}

			SubDesc = Desc->Get<PParams>(CStrID("UpdateValidator"), NULL);
			if (SubDesc.isvalid())
			{
				UpdateValidator = (CValidator*)CoreFct->Create(StrValidatorPfx + SubDesc->Get<nString>(CStrID("Type")));
				UpdateValidator->Init(SubDesc);
			}
		}

		OnStartCmd = CStrID(Desc->Get<nString>(CStrID("OnStartCmd"), nString::Empty).Get());
		OnDoneCmd = CStrID(Desc->Get<nString>(CStrID("OnDoneCmd"), nString::Empty).Get());
		OnEndCmd = CStrID(Desc->Get<nString>(CStrID("OnEndCmd"), nString::Empty).Get());
		OnAbortCmd = CStrID(Desc->Get<nString>(CStrID("OnAbortCmd"), nString::Empty).Get());

		SubDesc = Desc->Get<PParams>(CStrID("Preconditions"), NULL);
		if (SubDesc.isvalid())
		{
			Preconditions = (CWorldStateSource*)CoreFct->Create(StrWSSrcPfx + SubDesc->Get<nString>(CStrID("Type")));
			Preconditions->Init(SubDesc);
		}
	}
	else
	{
		Enabled =
		AppearsInUI = true;
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