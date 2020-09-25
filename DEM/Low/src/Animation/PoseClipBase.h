#pragma once
#include <Data/Ptr.h>

// A base class for clips on the pose timeline track

namespace DEM::Anim
{
using PPoseClipBase = std::unique_ptr<class CPoseClipBase>;
using PPoseOutput = Ptr<class IPoseOutput>;
class CPoseTrack;

class CPoseClipBase
{
protected:

	PPoseOutput _Output; // Track output (for direct port mapping) or mapped output

public:

	CPoseClipBase();
	virtual ~CPoseClipBase();

	virtual PPoseClipBase Clone() const = 0;
	virtual void          BindToOutput(const PPoseOutput& Output) = 0;
	virtual void          PlayInterval(float PrevTime, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR ClipIndex) = 0;
};

}
