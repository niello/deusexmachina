#pragma once
#include <Data/Ptr.h>

// A base class for clips on the pose timeline track

namespace DEM::Anim
{
using PPoseClipBase = std::unique_ptr<class CPoseClipBase>;
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
class IPoseOutput;

class CPoseClipBase
{
public:

	CPoseClipBase();
	virtual ~CPoseClipBase();

	virtual PPoseClipBase Clone() const = 0;
	virtual void          GatherSkeletonInfo(PSkeletonInfo& SkeletonInfo) = 0;
	virtual void          PlayInterval(float PrevTime, float CurrTime, bool IsLast, IPoseOutput& Output) = 0;
};

}
