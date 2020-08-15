#pragma once
#include <Data/Ptr.h>

// A base class for clips on the pose timeline track

namespace DEM::Anim
{
using PPoseOutput = Ptr<class IPoseOutput>;
class CPoseTrack;

class CPoseClipBase
{
protected:

	PPoseOutput _Output = nullptr; // Track output (for direct port mapping) or mapped output
	float       _Duration = 0.f;

public:

	float        GetDuration() const { return _Duration; }

	virtual void BindToOutput(const PPoseOutput& Output) = 0;
	virtual void PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex) = 0;
};

}
