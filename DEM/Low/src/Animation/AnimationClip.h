#pragma once
#include <Core/Object.h>

// ACL-compressed animation clip. Animates local SRT transform.

namespace acl
{
	class CompressedClip;
}

namespace DEM::Anim
{
using PNodeMapping = Ptr<class CNodeMapping>;

class CAnimationClip: public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Anim::CAnimationClip, ::Core::CObject);

protected:

	acl::CompressedClip* _pClip = nullptr;
	PNodeMapping         _NodeMapping;
	float                _Duration = 0.f;

public:

	CAnimationClip(acl::CompressedClip* pClip, float Duration, PNodeMapping&& NodeMapping);
	virtual ~CAnimationClip() override;

	const acl::CompressedClip* GetACLClip() const { return _pClip; }
	CNodeMapping& GetNodeMapping() const { return *_NodeMapping; } // non-const to create intrusive strong refs
	float GetDuration() const { return _Duration; }
	UPTR  GetNodeCount() const;
};

typedef Ptr<CAnimationClip> PAnimationClip;

}
