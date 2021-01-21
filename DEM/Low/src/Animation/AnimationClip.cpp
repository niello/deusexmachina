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

	// All locomotion info has meaning only with non-empty clips. Save us some checks in runtime.
	if (!_SampleCount) _LocomotionInfo.reset();
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

float CAnimationClip::GetLocomotionPhase(float NormalizedTime) const
{
	if (!_LocomotionInfo) return std::numeric_limits<float>().lowest();

	n_assert_dbg(NormalizedTime >= 0.f && NormalizedTime <= 1.f);

	const size_t SegmentCount = _SampleCount - 1;
	const float Sample = NormalizedTime * SegmentCount;
	float IntSampleF;
	const float Factor = std::modff(Sample, &IntSampleF);

	const size_t IntSample = static_cast<size_t>(IntSampleF);
	if (IntSample >= SegmentCount) return _LocomotionInfo->Phases[IntSample];
	return std::fmaf(_LocomotionInfo->Phases[IntSample], 1.f - Factor, Factor * _LocomotionInfo->Phases[IntSample + 1]);
}
//---------------------------------------------------------------------

float CAnimationClip::GetLocomotionPhaseNormalizedTime(float Phase) const
{
	if (!_LocomotionInfo) return std::numeric_limits<float>().lowest();

	n_assert_dbg(Phase >= 0.f && Phase <= 360.f);

	const auto& PhaseTimes = _LocomotionInfo->PhaseNormalizedTimes;
	const auto It = std::lower_bound(PhaseTimes.cbegin(), PhaseTimes.cend(), Phase,
		[](const auto& Elm, float Value) { return Elm.first < Value; });
	n_assert_dbg(It != PhaseTimes.cend());
	if (It == PhaseTimes.cend()) return std::numeric_limits<float>().lowest();

	// Sentinel frames eliminate iterator wrapping
	const auto PrevIt = std::prev(It);
	const float Time1 = PrevIt->second;
	float Time2 = It->second;
	const float Factor = (Phase - PrevIt->first) / (It->first - PrevIt->first);

	if (Time2 > Time1) return std::fmaf(Time1, 1.f - Factor, Factor * Time2);

	// Special case - looping from end to beginning
	Time2 += 1.f;
	const float Result = std::fmaf(Time1, 1.f - Factor, Factor * Time2);
	return (Result > 1.f) ? (Result - 1.f) : Result;
}
//---------------------------------------------------------------------

}
