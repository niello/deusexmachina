#include "TimeSource.h"

namespace Time
{
ImplementRTTI(Time::CTimeSource, Core::CRefCounted);
ImplementFactory(Time::CTimeSource);

CTimeSource::CTimeSource():
	FrameTime(0.001f),
	Time(0.0),
	PauseCounter(0),
	TimeFactor(1.0f),
	FrameID(0)
{
}
//---------------------------------------------------------------------

void CTimeSource::Update(nTime _FrameTime)
{
	++FrameID;
	if (!IsPaused())
	{
		FrameTime = _FrameTime * TimeFactor;
		Time += FrameTime;
	}
}
//---------------------------------------------------------------------

}
