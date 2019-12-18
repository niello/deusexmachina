#pragma once
#include <Resources/ResourceObject.h>

// ACL-compressed animation clip. Animates local SRT transform.

namespace acl
{
	class CompressedClip;
}

namespace DEM::Anim
{

class CAnimationClip: public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	acl::CompressedClip* _pClip = nullptr;

public:

	CAnimationClip(acl::CompressedClip* pClip);
	virtual ~CAnimationClip() override;

	virtual bool IsResourceValid() const override { return !!_pClip; }

	const acl::CompressedClip* GetACLClip() const { return _pClip; }
};

typedef Ptr<CAnimationClip> PAnimationClip;

}
