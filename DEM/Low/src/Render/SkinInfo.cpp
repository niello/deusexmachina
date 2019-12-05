#include "SkinInfo.h"

#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkinInfo, 'SKIF', Resources::CResourceObject);

bool CSkinInfo::Create(UPTR BoneCount)
{
	if (pInvBindPose) FAIL;
	pInvBindPose = (matrix44*)n_malloc_aligned(BoneCount * sizeof(matrix44), 16);
	if (!pInvBindPose) FAIL;
	Bones.SetSize(BoneCount);
	OK;
}
//---------------------------------------------------------------------

void CSkinInfo::Destroy()
{
	Bones.SetSize(0);
	SAFE_FREE_ALIGNED(pInvBindPose);
}
//---------------------------------------------------------------------

}