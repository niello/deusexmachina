#include "TimeSource.h"

namespace Core
{
__ImplementClassNoFactory(Core::CTimeSource, Core::CObject);

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
