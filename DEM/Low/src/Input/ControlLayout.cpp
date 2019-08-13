#include "ControlLayout.h"
#include <Input/InputConditionText.h>
#include <Input/InputConditionPressed.h>
#include <Input/InputConditionReleased.h>
#include <Input/InputConditionHold.h>
#include <Input/InputConditionMove.h>
#include <Input/InputConditionUp.h>
#include <Data/Params.h>
#include <cctype>

namespace Input
{

static Core::CRTTIBaseClass* ParseRule(const char*& pRule)
{
	const char* pCurr = pRule;
	while (pCurr)
	{
		char c = *pCurr;
		if (c == '\'')
		{
			const char* pEnd = strchr(pCurr + 1, '\'');
			if (!pEnd) return nullptr;
			pRule = pEnd;
			return new CInputConditionText(std::string(pCurr, pEnd));
		}
		else if (c == '|')
		{
			// parse next rule
			// combine
		}
		else if (std::isspace(c))
		{
			// Skip space
		}
		else if (c == '%' || c == '_' || std::isalnum(static_cast<unsigned char>(c)))
		{
			// TODO: can use std::isprint to parse key symbols like "~"

			const bool IsParam = (c == '%');
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
		else return nullptr;

		++pCurr;
	}

	return nullptr;
}
//---------------------------------------------------------------------

bool CControlLayout::Initialize(const Data::CParams& Desc)
{
	for (UPTR i = 0; i < Desc.GetCount(); ++i)
	{
		const Data::CParam& Prm = Desc.Get(i);

		const auto& RuleStr = Prm.GetValue<CString>();
		const char* pRuleStr = RuleStr.CStr();
		Core::CRTTIBaseClass* pRule(ParseRule(pRuleStr));
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