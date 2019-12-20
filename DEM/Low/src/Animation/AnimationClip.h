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

	constexpr static U16 NoParentIndex = std::numeric_limits<U16>().max();

	struct CNodeInfo
	{
		CStrID ID;
		U16    ParentIndex = NoParentIndex;
	};

protected:

	acl::CompressedClip*   _pClip = nullptr;
	std::vector<CNodeInfo> _NodeMapping;

public:

	CAnimationClip(acl::CompressedClip* pClip, std::vector<CNodeInfo>&& NodeMapping);
	virtual ~CAnimationClip() override;

	virtual bool IsResourceValid() const override { return !!_pClip; }

	const acl::CompressedClip* GetACLClip() const { return _pClip; }
	const UPTR GetNodeCount() const { return _NodeMapping.size(); }
	const CNodeInfo& GetNodeInfo(UPTR Index) const { return _NodeMapping[Index]; }
};

typedef Ptr<CAnimationClip> PAnimationClip;

}
