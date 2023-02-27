#include "SkinInfo.h"

#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkinInfo, 'SKIF', ::Core::CObject);

CSkinInfo::~CSkinInfo()
{
	Destroy();
}
//---------------------------------------------------------------------

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

UPTR CSkinInfo::GetBoneMatchingLength(const CSkinInfo& Other) const
{
	const UPTR MinCount = std::min(GetBoneCount(), Other.GetBoneCount());
	for (UPTR i = 0; i < MinCount; ++i)
	{
		const auto& OurBone = Bones[i];
		const auto& OtherBone = Other.Bones[i];
		if (OurBone.ID != OtherBone.ID || OurBone.ParentIndex != OtherBone.ParentIndex) return i;

		const matrix44& OurMatrix = pInvBindPose[i];
		const matrix44& OtherMatrix = Other.pInvBindPose[i];
		if (OurMatrix != OtherMatrix) return i;
	}

	return MinCount;
}
//---------------------------------------------------------------------

}
