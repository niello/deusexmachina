#include "StaticPose.h"
#include <Animation/SkeletonInfo.h>
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{

CStaticPose::CStaticPose(std::vector<rtm::qvvf>&& Transforms, PSkeletonInfo&& SkeletonInfo)
	: _Transforms(std::move(Transforms))
	, _SkeletonInfo(std::move(SkeletonInfo))
{
	n_assert_dbg(_SkeletonInfo->GetNodeCount() == _Transforms.size());
}
//---------------------------------------------------------------------

CStaticPose::~CStaticPose() = default;
//---------------------------------------------------------------------

void CStaticPose::Apply(IPoseOutput& Output)
{
	const auto Count = _Transforms.size();
	for (UPTR i = 0; i < Count; ++i)
		Output.SetTransform(i, _Transforms[i]);
}
//---------------------------------------------------------------------

}
