#include "TimeSource.h"

namespace DEM::Core
{

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
