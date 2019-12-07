#include "SkinInfo.h"

#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkinInfo, 'SKIF', Resources::CResourceObject);

bool CSkinInfo::Create(UPTR BoneCount)
{
	if (pInvBindPose) FAIL;
	pInvBindPose = static_cast<matrix44*>(n_malloc_aligned(BoneCount * sizeof(matrix44), 16));
	if (!pInvBindPose) FAIL;
	Bones.resize(BoneCount);
	OK;
}
//---------------------------------------------------------------------

void CSkinInfo::Destroy()
{
	Bones.clear();
	SAFE_FREE_ALIGNED(pInvBindPose);
}
//---------------------------------------------------------------------

}