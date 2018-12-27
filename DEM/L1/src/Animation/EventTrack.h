#pragma once
#ifndef __DEM_L1_ANIM_EVENT_TRACK_H__
#define __DEM_L1_ANIM_EVENT_TRACK_H__

#include <Animation/AnimFwd.h>
#include <Data/FixedArray.h>

// Event track places events on the animation timeline. Once time cursor passes the event time mark,
// event is fired with additional info about time direction (reverse or not) etc.

namespace Anim
{

class CEventTrack
{
public:

	struct CKey
	{
		float	Time;
		CStrID	EventID;
		//???Data::PParams Params;?

		bool operator >(const CKey& Other) const { return Time > Other.Time; }
		bool operator <(const CKey& Other) const { return Time < Other.Time; }
	};

	CFixedArray<CKey> Keys; // Must be sorted by time
};

}

#endif
