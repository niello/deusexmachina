#include "AnimationClip.h"
#include <acl/core/compressed_clip.h>

namespace DEM::Anim
{
RTTI_CLASS_IMPL(DEM::Anim::CAnimationClip, Resources::CResourceObject);

CAnimationClip::CAnimationClip(acl::CompressedClip* pClip, float Duration, std::vector<CNodeInfo>&& NodeMapping)
	: _pClip(pClip)
	, _NodeMapping(std::move(NodeMapping))
	, _Duration(Duration)
{
	// DEM animation format forces the root to be at position 0
	n_assert(!_NodeMapping.empty() && _NodeMapping.front().ParentIndex == NoParentIndex);
}
//---------------------------------------------------------------------

CAnimationClip::~CAnimationClip()
{
	SAFE_FREE_ALIGNED(_pClip);
}
//---------------------------------------------------------------------

}
