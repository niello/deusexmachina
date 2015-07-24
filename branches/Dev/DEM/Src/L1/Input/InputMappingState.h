#pragma once
#ifndef __DEM_L1_INPUT_MAPPING_STATE_H__
#define __DEM_L1_INPUT_MAPPING_STATE_H__

#include <Core/Object.h>
#include <Events/EventsFwd.h>
#include <Data/StringID.h>
#include <Data/Array.h>
#include <Input/Keys.h>

// Input state mapping maps set of input state conditions to abstract global state,
// setting it to true on all conditions are met and to false otherwise. Mapping can
// specify requirement for conditions to be met in specific or any order. Also this
// mapping can send events on its overall state change.
// NB: you can't map mouse movement cause it has no state. Wheel can be mapped.

namespace Data
{
	class CParams;
}

namespace Input
{

class CInputMappingState //: public Core::CObject
{
private:

	//!!!Can add longpressed etc!
	enum EConditionType
	{
		CT_Key,
		CT_MouseBtn,
		CT_Wheel
	};

	//!!!can pack enums to 1 byte! now this struct size is 12, but can be 3(4)
	struct CCondition
	{
		EConditionType	Type;
		union
		{
			struct
			{
				union
				{
					EKey			Key;
					EMouseButton	MouseBtn;
				};
				uchar KeyBtnState;
			};
			bool WheelFwd;
		};
		bool			State;
	};

	//!!!to char flags!
	bool				CheckInOrder;
	bool				SendStateChangeEvent;
	bool				State;

	CStrID				Name;
	CStrID				EventOn;
	CStrID				EventOff;

	CArray<CCondition>	Conditions;

	DECLARE_EVENT_HANDLER(OnInputUpdated, OnInputUpdated);

	//???need to listen ResetInput or next frame will update all properly?

public:

	CInputMappingState();

	bool Init(CStrID Name, const Data::CParams& Desc);
	void Enable();
	void Disable();
	bool IsEnabled() const { return IS_SUBSCRIBED(OnInputUpdated); }
	bool IsActive() const { return State; }
};

}

#endif
