#include "ControlLayout.h"
#include <Data/Params.h>

namespace Input
{

bool CControlLayout::Initialize(const Data::CParams& Desc)
{
	Clear();

	for (UPTR i = 0; i < Desc.GetCount(); ++i)
	{
		const Data::CParam& Prm = Desc.Get(i);

		const auto& RuleStr = Prm.GetValue<std::string>();
		if (RuleStr.empty()) continue;

		const char* pRuleStr = RuleStr.c_str();
		auto pRule = ParseRule(pRuleStr);
		if (!pRule)
		{
			::Sys::Error("CControlLayout::Initialize() > error parsing rule string \"{}\"\n"_format(RuleStr));
			continue;
		}

		if (auto pConditionEvent = pRule->As<CInputConditionEvent>())
		{
			CEventRecord NewRec;
			NewRec.OutEventID = Prm.GetName();
			NewRec.Event.reset(pConditionEvent);
			Events.push_back(std::move(NewRec));
		}
		else if (auto pConditionState = pRule->As<CInputConditionState>())
		{
			States.emplace(Prm.GetName(), pConditionState);
		}
		else
		{
			n_delete(pRule);
			::Sys::Error("CControlLayout::Initialize() > unexpected rule type returned for \"{}\"\n"_format(RuleStr));
			continue;
		}
	}

	OK;
}
//---------------------------------------------------------------------

void CControlLayout::Clear()
{
	States.clear();
	Events.clear();
}
//---------------------------------------------------------------------

void CControlLayout::Reset()
{
	for (auto& Pair : States) Pair.second->Reset();
	for (auto& Rec : Events) Rec.Event->Reset();
}
//---------------------------------------------------------------------

}
