#include "AnimationClip.h"
#include <Animation/SceneNodeMapping.h>
#include <acl/core/compressed_clip.h>

namespace DEM::Anim
{

CAnimationClip::CAnimationClip(acl::CompressedClip* pClip, float Duration, PSceneNodeMapping&& NodeMapping)
	: _pClip(pClip)
	, _NodeMapping(std::move(NodeMapping))
	, _Duration(Duration)
{
	// DEM animation format forces the root to be at position 0
	n_assert(_NodeMapping && _NodeMapping->GetNodeCount() && _NodeMapping->GetNodeInfo(0).ParentIndex == CSceneNodeMapping::NoParentIndex);
}
//---------------------------------------------------------------------

CAnimationClip::~CAnimationClip()
{
	SAFE_FREE_ALIGNED(_pClip);
}
//---------------------------------------------------------------------

UPTR CAnimationClip::GetNodeCount() const
{
	return _NodeMapping->GetNodeCount();
}
//---------------------------------------------------------------------

}
