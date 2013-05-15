#pragma once
#ifndef __DEM_L1_EVENT_PLAY_SOUND_H__
#define __DEM_L1_EVENT_PLAY_SOUND_H__

#include <Events/EventNative.h>

#ifdef PlaySound
#undef PlaySound
#endif

// Play a sound at the specific position.

namespace Event
{

class PlaySound: public Events::CEventNative
{
	__DeclareClass(PlaySound);

public:

	nString	Name;
	vector3	Position;
	vector3	Velocity;
	float	Volume;

	PlaySound(): Volume(1.0f) {}
};

__RegisterClassInFactory(PlaySound);

}

#endif
