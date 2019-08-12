#pragma once
#include <Core/Object.h>
#include <Input/InputConditionEvent.h>
#include <Input/InputConditionState.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>

// Control layout is a set of mappings that generate events or switch states
// in response to incoming input events.

namespace Data
{
	class CParams;
}

namespace Input
{

class CControlLayout final
{
public:

	struct CEventRecord
	{
		CStrID					OutEventID;
		CInputConditionEvent*	pEvent;
	};

	CArray<CEventRecord>					Events; // Order is important, so we don't use dictionary
	CDict<CStrID, CInputConditionState*>	States;	// Order is not important

	CControlLayout() { Events.SetKeepOrder(true); }
	~CControlLayout() { Clear(); }

	bool Initialize(const Data::CParams& Desc);
	void Clear();
	void Reset();
};

}
