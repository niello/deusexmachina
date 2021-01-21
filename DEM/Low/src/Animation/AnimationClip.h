#pragma once
#include <Core/Object.h>
#include <map>

// ACL-compressed animation clip. Animates local SRT transform.

namespace acl
{
	class CompressedClip;
}

namespace DEM::Anim
{
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
using PBipedLocomotionInfo = std::unique_ptr<struct CBipedLocomotionInfo>;

struct CBipedLocomotionInfo
{
	std::unique_ptr<float[]>             Phases; // One per each sample in a clip
	std::vector<std::pair<float, float>> PhaseNormalizedTimes;
	float                                Speed = 0.f;
	U32                                  LeftFootOnGroundFrame = std::numeric_limits<U32>().max();
	U32                                  RightFootOnGroundFrame = std::numeric_limits<U32>().max();
};

class CAnimationClip: public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Anim::CAnimationClip, ::Core::CObject);

protected:

	acl::CompressedClip* _pClip = nullptr;
	PSkeletonInfo        _SkeletonInfo;
	PBipedLocomotionInfo _LocomotionInfo;
	float                _Duration = 0.f;
	U32                  _SampleCount = 0; // TODO: can get from CompressedClip

public:

	CAnimationClip(acl::CompressedClip* pClip, float Duration, U32 SampleCount, PSkeletonInfo&& SkeletonInfo, PBipedLocomotionInfo&& LocomotionInfo = nullptr);
	virtual ~CAnimationClip() override;

	const acl::CompressedClip*  GetACLClip() const { return _pClip; }
	CSkeletonInfo&              GetSkeletonInfo() const { return *_SkeletonInfo; } // non-const to create intrusive strong refs
	const CBipedLocomotionInfo* GetLocomotionInfo() const { return _LocomotionInfo.get(); }
	float                       GetDuration() const { return _Duration; }
	U32                         GetSampleCount() const { return _SampleCount; }
	UPTR                        GetNodeCount() const;
	float                       GetLocomotionPhase(float NormalizedTime) const;
	float                       GetLocomotionPhaseNormalizedTime(float Phase) const;

	float AdjustTime(float Time, bool Loop) const
	{
		if (Time < 0.f) return Loop ? _Duration + std::fmodf(Time, _Duration) : 0.f;
		else if (Time > _Duration) return Loop ? std::fmodf(Time, _Duration) : _Duration;
		else return Time;
	}
};

typedef Ptr<CAnimationClip> PAnimationClip;

}
