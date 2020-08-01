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

	PPoseOutput _Output = nullptr; // Track output (for direct port mapping) or mapped output

public:

	virtual void BindToOutput(/*PPoseOutput&& Output*/) = 0;
	virtual void PlayInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) = 0;
};

}
