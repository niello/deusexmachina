#pragma once
#ifndef __DEM_L1_INPUT_CONTROL_LAYOUT_H__
#define __DEM_L1_INPUT_CONTROL_LAYOUT_H__

#include <Core/Object.h>
#include <Input/InputCondition.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>

// Control layout is a set of mappings, each of which translates input
// into abstract events and states.

namespace Input
{

class CControlLayout: public Core::CObject
{
private:

	CArray<CInputConditionEvent*>			Events; //???event IDs inside? processing order is important
	CDict<CStrID, CInputConditionState*>	States;	// Order is not important

public:

	~CControlLayout(); //!!!n_delete() conditions!

	bool Initialize(const Data::CParams& Desc);
	void Reset();
};

typedef Ptr<CControlLayout> PControlLayout;

}

#endif
