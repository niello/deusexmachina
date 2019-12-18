#include "AnimationClip.h"

namespace DEM::Anim
{
RTTI_CLASS_IMPL(DEM::Anim::CAnimationClip, Resources::CResourceObject);

CAnimationClip::CAnimationClip(acl::CompressedClip* pClip)
	: _pClip(pClip)
{
}
//---------------------------------------------------------------------

CAnimationClip::~CAnimationClip()
{
	SAFE_FREE_ALIGNED(_pClip);
}
//---------------------------------------------------------------------

}
