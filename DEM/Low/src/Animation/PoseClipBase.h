#pragma once
#include <StdDEM.h>

// A base class for clips on the pose timeline track

namespace DEM::Anim
{
using PPoseOutput = std::unique_ptr<class IPoseOutput>;
class CPoseTrack;

class CPoseClipBase
{
protected:

	PPoseOutput Output; // Track output (for direct port mapping) or mapped output

public:

	//virtual BindToOutput

	virtual void PlayInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) = 0;
};

}
