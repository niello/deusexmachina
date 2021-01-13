#include "AnimatedPoseClip.h"
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/MappedPoseOutput.h>
#include <Animation/PoseTrack.h>

namespace DEM::Anim
{

void CAnimatedPoseClip::SetAnimationClip(const PAnimationClip& Clip)
{
	_Sampler.SetClip(Clip);
}
//---------------------------------------------------------------------

PPoseClipBase CAnimatedPoseClip::Clone() const
{
	PAnimatedPoseClip NewClip(n_new(CAnimatedPoseClip()));
	NewClip->SetAnimationClip(_Sampler.GetClip());

	// NB: sampler state (curr time etc) is not cloned now. Need?

	return NewClip;
}
//---------------------------------------------------------------------

void CAnimatedPoseClip::GatherSkeletonInfo(PSkeletonInfo& SkeletonInfo)
{
	if (!_Sampler.GetClip()) return;
	CSkeletonInfo::Combine(SkeletonInfo, _Sampler.GetClip()->GetSkeletonInfo(), _PortMapping);
}
//---------------------------------------------------------------------

void CAnimatedPoseClip::PlayInterval(float /*PrevTime*/, float CurrTime, bool IsLast, const CPoseTrack& Track, UPTR /*ClipIndex*/)
{
	//!!!FIXME: sample pose only if CurrTime is inside the clip (>=0 && <= duration)! Otherwise clip is skipped already.
	//???IsLast will already check it? Last clip is always current, otherwise it is skipped. Rename to IsCurr?
	if (IsLast && Track.GetOutput())
	{
		if (_PortMapping)
		{
			CStackMappedPoseOutput MappedOutput(*Track.GetOutput(), _PortMapping.get());
			_Sampler.Apply(CurrTime, MappedOutput);
		}
		else
		{
			_Sampler.Apply(CurrTime, *Track.GetOutput());
		}
	}
}
//---------------------------------------------------------------------

}
