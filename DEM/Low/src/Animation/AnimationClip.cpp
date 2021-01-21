#include "AnimationClip.h"
#include <Animation/SkeletonInfo.h>
#include <acl/core/compressed_clip.h>

namespace DEM::Anim
{

CAnimationClip::CAnimationClip(acl::CompressedClip* pClip, float Duration, U32 SampleCount, PSkeletonInfo&& SkeletonInfo, PBipedLocomotionInfo&& LocomotionInfo)
	: _pClip(pClip)
	, _SkeletonInfo(std::move(SkeletonInfo))
	, _LocomotionInfo(std::move(LocomotionInfo))
	, _Duration(Duration)
	, _SampleCount(SampleCount)
{
	// TODO: compare _SkeletonInfo for equality (in tools?), share between clips where identical!

	// DEM animation format forces the root to be at position 0 and with empty ID
	n_assert(_SkeletonInfo &&
		_SkeletonInfo->GetNodeCount() &&
		_SkeletonInfo->GetNodeInfo(0).ParentIndex == CSkeletonInfo::EmptyPort &&
		!_SkeletonInfo->GetNodeInfo(0).ID);
}
//---------------------------------------------------------------------

CAnimationClip::~CAnimationClip()
{
	SAFE_FREE_ALIGNED(_pClip);
}
//---------------------------------------------------------------------

UPTR CAnimationClip::GetNodeCount() const
{
	return _SkeletonInfo->GetNodeCount();
}
//---------------------------------------------------------------------

}
