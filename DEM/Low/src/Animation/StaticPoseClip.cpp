#include "StaticPoseClip.h"
#include <Animation/SkeletonInfo.h>
#include <Animation/MappedPoseOutput.h>
#include <Animation/PoseTrack.h>

namespace DEM::Anim
{

PPoseClipBase CStaticPoseClip::Clone() const
{
	//!!!pose must be reusable, not unique ptr!
	//PStaticPoseClip NewClip(n_new(CStaticPoseClip()));
	//NewClip->_Pose = _Pose;
	//return NewClip;
	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

void CStaticPoseClip::GatherSkeletonInfo(PSkeletonInfo& SkeletonInfo)
{
	if (!_Pose) return;
	CSkeletonInfo::Combine(SkeletonInfo, _Pose->GetSkeletonInfo(), _PortMapping);
}
//---------------------------------------------------------------------

void CStaticPoseClip::PlayInterval(float /*PrevTime*/, float /*CurrTime*/, bool IsLast, const CPoseTrack& Track, UPTR /*ClipIndex*/)
{
	if (IsLast && _Pose && Track.GetOutput())
	{
		if (_PortMapping)
		{
			CStackMappedPoseOutput MappedOutput(*Track.GetOutput(), _PortMapping.get());
			_Pose->Apply(MappedOutput);
		}
		else
		{
			_Pose->Apply(*Track.GetOutput());
		}
	}
}
//---------------------------------------------------------------------

}
