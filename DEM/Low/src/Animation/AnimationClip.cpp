#include "AnimationClip.h"

namespace DEM::Anim
{
RTTI_CLASS_IMPL(DEM::Anim::CAnimationClip, Resources::CResourceObject);

CAnimationClip::CAnimationClip(acl::CompressedClip* pClip, std::vector<CNodeInfo>&& NodeMapping)
	: _pClip(pClip)
	, _NodeMapping(std::move(NodeMapping))
{
}
//---------------------------------------------------------------------

CAnimationClip::~CAnimationClip()
{
	SAFE_FREE_ALIGNED(_pClip);
}
//---------------------------------------------------------------------

}
