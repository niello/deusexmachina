#pragma once
#include <Resources/ResourceObject.h>
#include <Data/StringID.h>

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

public:

	struct CNodeInfo
	{
		CStrID Name;
		uint16_t ParentIndex = -1;
	};

protected:

	acl::CompressedClip*   _pClip = nullptr;
	std::vector<CNodeInfo> _NodeMapping;

public:

	CAnimationClip(acl::CompressedClip* pClip, std::vector<CNodeInfo>&& NodeMapping);
	virtual ~CAnimationClip() override;

	virtual bool IsResourceValid() const override { return !!_pClip; }

	const acl::CompressedClip* GetACLClip() const { return _pClip; }
};

typedef Ptr<CAnimationClip> PAnimationClip;

}
