#pragma once
#ifndef __DEM_L1_EVENT_CHAR_INPUT_H__
#define __DEM_L1_EVENT_CHAR_INPUT_H__

#include <Events/EventNative.h>

// Input event, representing a character input in UTF-8 encoding

namespace Event
{

//???inherit from InputEvent where DeviceID is stored?
class CharInput: public Events::CEventNative
{
	DeclareRTTI;

public:

	int Char;	// Now UTF-16 as comes from display

	CharInput(int _Char): Char(_Char) {}
};

}

#endif
