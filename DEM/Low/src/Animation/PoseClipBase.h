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

	PPoseOutput Output; //if mapping is direct, this is a track output!
	//optional mapping right here? Or mapped/non-mapped output subclasses?

public:

	virtual void UpdateInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex) = 0;
};

}
