#pragma once
#ifndef __DEM_L1_EVENT_KEY_DOWN_H__
#define __DEM_L1_EVENT_KEY_DOWN_H__

#include <Events/EventNative.h>
#include <Input/Keys.h>

// Input event

namespace Event
{

//???inherit from InputEvent where DeviceID is stored?
class KeyDown: public Events::CEventNative
{
	__DeclareClassNoFactory;

public:

	Input::EKey ScanCode;
	//???!!!bool IsFirst/IsRepeat;?!

	KeyDown(Input::EKey _ScanCode): ScanCode(_ScanCode) {}
};

}

#endif
