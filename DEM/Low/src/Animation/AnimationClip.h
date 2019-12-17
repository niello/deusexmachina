#pragma once
#include <Resources/ResourceObject.h>

// ACL-compressed animation clip. Animates local SRT transform.

namespace acl
{
	class CompressedClip;
}

namespace DEM::Anim
{

class CAnimClip: public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	const acl::CompressedClip* pClip = nullptr;

public:

	//
};

}
