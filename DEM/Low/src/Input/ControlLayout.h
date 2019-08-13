#pragma once
#include <Input/InputConditionEvent.h>
#include <Input/InputConditionState.h>
#include <Data/StringID.h>
#include <map>

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
		PInputConditionEvent	Event;
	};

	std::vector<CEventRecord>				Events; // Order is important, so we don't use dictionary
	std::map<CStrID, PInputConditionState>	States;	// Order is not important

	bool Initialize(const Data::CParams& Desc);
	void Clear();
	void Reset();
};

}
