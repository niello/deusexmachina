#pragma once
#include <Core/Object.h>

// ACL-compressed animation clip. Animates local SRT transform.

namespace acl
{
	class CompressedClip;
}

namespace DEM::Anim
{
using PSkeletonInfo = Ptr<class CSkeletonInfo>;

class CAnimationClip: public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Anim::CAnimationClip, ::Core::CObject);

protected:

	acl::CompressedClip* _pClip = nullptr;
	PSkeletonInfo        _SkeletonInfo;
	float                _Duration = 0.f;

public:

	CAnimationClip(acl::CompressedClip* pClip, float Duration, PSkeletonInfo&& SkeletonInfo);
	virtual ~CAnimationClip() override;

	const acl::CompressedClip* GetACLClip() const { return _pClip; }
	CSkeletonInfo&             GetSkeletonInfo() const { return *_SkeletonInfo; } // non-const to create intrusive strong refs
	float                      GetDuration() const { return _Duration; }
	UPTR                       GetNodeCount() const;

	float AdjustTime(float Time, bool Loop) const
	{
		if (Time < 0.f) return Loop ? _Duration + std::fmodf(Time, _Duration) : 0.f;
		else if (Time > _Duration) return Loop ? std::fmodf(Time, _Duration) : _Duration;
		else return Time;
	}
};

typedef Ptr<CAnimationClip> PAnimationClip;

}
