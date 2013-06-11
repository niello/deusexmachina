#include "TimeSource.h"

namespace Time
{
__ImplementClassNoFactory(Time::CTimeSource, Core::CRefCounted);

void CTimeSource::Update(float _FrameTime)
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
