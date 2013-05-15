#pragma once
#ifndef __DEM_L1_INPUT_MAPPING_EVENT_H__
#define __DEM_L1_INPUT_MAPPING_EVENT_H__

//#include <Core/RefCounted.h>
#include <Core/RTTI.h>
#include <Events/Events.h>
#include <Data/StringID.h>
#include <Input/Keys.h>

// Input event mapping maps input event to abstract global event,
// firing it on corresponding input event received.
// NB: you can't map mouse movement events. Wheel can be interpreted as a button due
// to its discrete steps, whereas X & Y axes can't. So, if you want to react on mouse
// movement, listen MouseMove[Raw] and implement shift treshold if necessary.

namespace Data
{
	class CParams;
}

namespace Input
{

class CInputMappingEvent //: public Core::CRefCounted
{
private:

	const Core::CRTTI*	InEventID;
	CStrID				OutEventID;

	// Condition
	union
	{
		EKey			Key;
		ushort			Char;
		EMouseButton	MouseBtn;
		bool			WheelFwd;	// true - forward, false - backward
	};

	Events::PSub		Sub_InputEvent;

	bool OnKeyDown(const Events::CEventBase& Event);
	bool OnKeyUp(const Events::CEventBase& Event);
	bool OnCharInput(const Events::CEventBase& Event);
	bool OnMouseBtnDown(const Events::CEventBase& Event);
	bool OnMouseBtnUp(const Events::CEventBase& Event);
	bool OnMouseDoubleClick(const Events::CEventBase& Event);
	bool OnMouseWheel(const Events::CEventBase& Event);

public:

	CInputMappingEvent(): InEventID(NULL) {}

	bool Init(CStrID Name, const Data::CParams& Desc);
	bool Init(CStrID Name, int KeyCode);
	void Enable();
	void Disable();
	bool IsEnabled() const { return Sub_InputEvent.IsValid(); }
};

}

#endif
