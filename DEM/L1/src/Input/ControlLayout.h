#pragma once
#ifndef __DEM_L1_INPUT_CONTROL_LAYOUT_H__
#define __DEM_L1_INPUT_CONTROL_LAYOUT_H__

#include <Core/Object.h>
#include <Input/InputCondition.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>

// Control layout is a set of mappings that generate events or switch states
// in response to incoming input events.

namespace Data
{
	class CDataArray;
}

namespace Input
{

class CControlLayout
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

	bool Initialize(const Data::CDataArray& Desc);
	void Clear();
	void Reset();
};

}

#endif
