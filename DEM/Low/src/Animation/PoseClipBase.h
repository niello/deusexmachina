#pragma once
#include <StdDEM.h>

// A base class for clips on the pose timeline track

namespace DEM::Anim
{
class IPoseOutput;
class CPoseTrack;

class CPoseClipBase
{
protected:

	IPoseOutput* _pOutput = nullptr; // Track output (for direct port mapping) or mapped output

public:

	virtual void BindToOutput(IPoseOutput* pOutput) = 0;
	virtual void PlayInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) = 0;
};

}
