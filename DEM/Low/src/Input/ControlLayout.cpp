#include "ControlLayout.h"
#include <Input/InputConditionText.h>
#include <Input/InputConditionPressed.h>
#include <Input/InputConditionReleased.h>
#include <Input/InputConditionHold.h>
#include <Input/InputConditionMove.h>
#include <Input/InputConditionUp.h>
#include <Input/InputConditionSequence.h>
#include <Input/InputConditionComboState.h>
#include <Input/InputConditionComboEvent.h>
#include <Input/InputConditionAnyOfEvents.h>
#include <Input/InputConditionAnyOfStates.h>
#include <Data/Params.h>
#include <cctype>

namespace Input
{

static Core::CRTTIBaseClass* ParseCondition(const char*& pRule)
{
	const char* pCurr = pRule;

	// Skip leading whitespace
	while (std::isspace(*pCurr)) ++pCurr;

	char c = *pCurr;

	// Rule is empty, it is an error
	if (!c || c == '|') return nullptr;

	// Text condition
	if (c == '\'')
	{
		++pCurr;
		const char* pEnd = strchr(pCurr, '\'');
		if (!pEnd) return nullptr;
		pRule = pEnd + 1;
		return new CInputConditionText(std::string(pCurr, pEnd));
	}

	// Axis or button condition
	// TODO: can use std::isprint to parse key symbols like "~"
	if (c == '$' || c == '_' || std::isalnum(static_cast<unsigned char>(c)))
	{
		const bool IsParam = (c == '$');
		if (IsParam) ++pCurr;

		const char* pEnd = pCurr;
		c = *pEnd;
		while (c == '_' || std::isalnum(static_cast<unsigned char>(c)))
		{
			++pEnd;
			c = *pEnd;
		}

		std::string ID(pCurr, pEnd);

		if (IsParam)
		{
			//???how to determine input device class?
			//???resolve current value? needs user as context!
			//???or needs parametrized rule?
			NOT_IMPLEMENTED;
			return nullptr;
		}

		// Resolve input channel from ID
		EDeviceType DeviceType;
		bool IsAxis = false;
		U8 Code = static_cast<U8>(StringToKey(ID.c_str()));
		if (Code != InvalidCode) DeviceType = Device_Keyboard;
		else
		{
			Code = static_cast<U8>(StringToMouseButton(ID.c_str()));
			if (Code != InvalidCode) DeviceType = Device_Mouse;
			else
			{
				Code = static_cast<U8>(StringToMouseAxis(ID.c_str()));
				if (Code != InvalidCode)
				{
					DeviceType = Device_Mouse;
					IsAxis = true;
				}
				else return nullptr; // TODO: gamepad
			}
		}

		// Process optional modifiers
		switch (c)
		{
			case '+':
			{
				if (IsAxis) return nullptr;
				pRule = pEnd + 1;
				return new CInputConditionPressed(DeviceType, Code);
			}
			case '-':
			{
				if (IsAxis) return nullptr;
				pRule = pEnd + 1;
				return new CInputConditionReleased(DeviceType, Code);
			}
			case '[':
			case '{':
			{
				const bool Repeat = (c == '{');
				pCurr = pEnd + 1;
				pEnd = strchr(pCurr, Repeat ? '}' : ']');
				if (!pEnd) return nullptr;

				char* pParseEnd = nullptr;
				const float TimeOrAmount = strtof(pCurr, &pParseEnd);
				if (pParseEnd != pEnd) return nullptr;

				pRule = pEnd + 1;

				if (IsAxis)
					return new CInputConditionMove(DeviceType, Code, TimeOrAmount);
				else
					return new CInputConditionHold(DeviceType, Code, TimeOrAmount, Repeat);
			}
			default:
			{
				pRule = pEnd;

				if (IsAxis)
					return new CInputConditionMove(DeviceType, Code);
				else
					return new CInputConditionUp(DeviceType, Code);
			}
		}
	}

	// Unknown condition type
	return nullptr;
}
//---------------------------------------------------------------------

static Core::CRTTIBaseClass* ParseSequence(const char*& pRule)
{
	Core::CRTTIBaseClass* pResult = nullptr;

	while (*pRule && *pRule != '|')
	{
		Core::CRTTIBaseClass* pCondition = ParseCondition(pRule);

		// Failed parsing is an error, result is discarded
		if (!pCondition)
		{
			n_delete(pResult);
			return nullptr;
		}

		if (!pResult) pResult = pCondition;
		else
		{
			const bool FirstIsEvent = pResult->IsA<CInputConditionEvent>();
			const bool SecondIsEvent = pCondition->IsA<CInputConditionEvent>();
			if (FirstIsEvent && SecondIsEvent)
			{
				// Events are combined into sequences
				auto pSequence = pResult->As<CInputConditionSequence>();
				if (!pSequence)
				{
					pSequence = new CInputConditionSequence();
					pSequence->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pResult)));
					pResult = pSequence;
				}
				pSequence->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pCondition)));
			}
			else if (!FirstIsEvent && !SecondIsEvent)
			{
				// States are combined into one combo state
				auto pCombo = pResult->As<CInputConditionComboState>();
				if (!pCombo)
				{
					pCombo = new CInputConditionComboState();
					pCombo->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pResult)));
					pResult = pCombo;
				}
				pCombo->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pCondition)));
			}
			else
			{
				// Events and states are mixed through the combo event. Once it is created, all events
				// are combined into one sequence and all states into one combo state inside a combo event.

				auto pEvent = FirstIsEvent ? pResult : pCondition;
				auto pState = FirstIsEvent ? pCondition : pResult;

				auto pCombo = pEvent->As<CInputConditionComboEvent>();
				if (!pCombo)
				{
					pCombo = new CInputConditionComboEvent();
					pCombo->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pEvent)));
				}
				pCombo->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pState)));

				pResult = pCombo;
			}
		}

		// Skip trailing whitespace
		while (std::isspace(*pRule)) ++pRule;
	}

	return pResult;
}
//---------------------------------------------------------------------

static Core::CRTTIBaseClass* ParseRule(const char* pRule)
{
	Core::CRTTIBaseClass* pResult = nullptr;
	bool IsSequence = true;

	do
	{
		Core::CRTTIBaseClass* pCondition = ParseSequence(pRule);

		if (!pCondition)
		{
			// Failed parsing is an error, result is discarded
			n_delete(pResult);
			return nullptr;
		}

		if (!pResult) pResult = pCondition;
		else
		{
			const bool FirstIsEvent = pResult->IsA<CInputConditionEvent>();
			const bool SecondIsEvent = pCondition->IsA<CInputConditionEvent>();
			if (FirstIsEvent != SecondIsEvent)
			{
				// Can't mix events and states with OR
				n_delete(pResult);
				return nullptr;
			}

			if (FirstIsEvent)
			{
				auto pAny = pResult->As<CInputConditionAnyOfEvents>();
				if (!pAny)
				{
					pAny = new CInputConditionAnyOfEvents();
					pAny->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pResult)));
					pResult = pAny;
				}
				pAny->AddChild(PInputConditionEvent(static_cast<CInputConditionEvent*>(pCondition)));
			}
			else
			{
				auto pAny = pResult->As<CInputConditionAnyOfStates>();
				if (!pAny)
				{
					pAny = new CInputConditionAnyOfStates();
					pAny->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pResult)));
					pResult = pAny;
				}
				pAny->AddChild(PInputConditionState(static_cast<CInputConditionState*>(pCondition)));
			}
		}

		// Skip trailing whitespace
		while (std::isspace(*pRule)) ++pRule;
	}
	while (*pRule++ == '|');

	return pResult;
}
//---------------------------------------------------------------------

bool CControlLayout::Initialize(const Data::CParams& Desc)
{
	Clear();

	for (UPTR i = 0; i < Desc.GetCount(); ++i)
	{
		const Data::CParam& Prm = Desc.Get(i);

		const auto& RuleStr = Prm.GetValue<CString>();
		if (RuleStr.IsEmpty()) continue;

		const char* pRuleStr = RuleStr.CStr();
		auto pRule = ParseRule(pRuleStr);
		if (!pRule)
		{
			::Sys::Error("CControlLayout::Initialize() > error parsing rule string \"" + RuleStr + "\"\n");
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
			::Sys::Error("CControlLayout::Initialize() > unexpected rule type returned for \"" + RuleStr + "\"\n");
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