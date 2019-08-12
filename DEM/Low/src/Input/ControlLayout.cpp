#include "ControlLayout.h"
#include <Data/Params.h>
#include <cctype>

namespace Input
{

static bool ParseRule(const CString& Rule)
{
	const char* pCurr = Rule.CStr();
	while (pCurr)
	{
		char c = *pCurr;
		if (c == '\'')
		{
			const char* pEnd = strchr(pCurr + 1, '\'');
			if (!pEnd) FAIL;

			// create text input condition
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
		else if (c == '%' || c == '_' || std::isalnum(c))
		{
			const bool IsParam = (c == '%');
			if (IsParam) ++pCurr;

			const char* pEnd = pCurr;
			c = *pEnd;
			while (c == '_' || std::isalnum(c))
			{
				++pEnd;
				c = *pEnd;
			}

			std::string ID(pCurr, pEnd);

			// Process optional modifiers
			switch (c)
			{
				case '+': break;
				case '-': break;
				case '[': break;
				case '{': break;
			}
		}
		else FAIL;

		++pCurr;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CControlLayout::Initialize(const Data::CParams& Desc)
{
	States.BeginAdd();

	for (UPTR i = 0; i < Desc.GetCount(); ++i)
	{
		const Data::CParam& Prm = Desc.Get(i);

		if (!ParseRule(Prm.GetValue<CString>())) continue;

		//

		/*
		{
			CEventRecord& NewRec = *Events.Add();
			NewRec.OutEventID = Prm.GetName();
			NewRec.pEvent = CInputConditionEvent::CreateByType(ConditionType);
			if (!NewRec.pEvent || !NewRec.pEvent->Initialize(*MappingDesc.Get())) FAIL;
		}
		{
			CInputConditionState* pState = CInputConditionState::CreateByType(ConditionType);
			if (!pState || !pState->Initialize(*MappingDesc.Get())) FAIL;
			States.Add(Prm.GetName(), pState);
		}
		*/
	}

	States.EndAdd();

	OK;
}
//---------------------------------------------------------------------

void CControlLayout::Clear()
{
	for (CDict<CStrID, CInputConditionState*>::CIterator It = States.Begin(); It != States.End(); ++It)
		n_delete(It->GetValue());
	States.Clear();
	for (CArray<CEventRecord>::CIterator It = Events.Begin(); It != Events.End(); ++It)
		n_delete(It->pEvent);
	Events.Clear();
}
//---------------------------------------------------------------------

void CControlLayout::Reset()
{
	for (CDict<CStrID, CInputConditionState*>::CIterator It = States.Begin(); It != States.End(); ++It)
		It->GetValue()->Reset();
	for (CArray<CEventRecord>::CIterator It = Events.Begin(); It != Events.End(); ++It)
		It->pEvent->Reset();
}
//---------------------------------------------------------------------

}