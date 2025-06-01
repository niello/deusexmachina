#include "SkinInfo.h"
#include <Math/SIMDMath.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkinInfo, 'SKIF', DEM::Core::CObject);

CSkinInfo::~CSkinInfo()
{
	Destroy();
}
//---------------------------------------------------------------------

bool CSkinInfo::Create(UPTR BoneCount)
{
	if (pInvBindPose) FAIL;
	pInvBindPose = static_cast<rtm::matrix3x4f*>(n_malloc_aligned(BoneCount * sizeof(rtm::matrix3x4f), alignof(rtm::matrix3x4f)));
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
		if (OurBone.ID != OtherBone.ID || OurBone.ParentIndex != OtherBone.ParentIndex || !Math::matrix_all_equal(pInvBindPose[i], Other.pInvBindPose[i])) return i;
	}

	return MinCount;
}
//---------------------------------------------------------------------

}
