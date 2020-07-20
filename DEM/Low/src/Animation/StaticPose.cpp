#include "StaticPose.h"
#include <Animation/AnimationBlender.h>
#include <Animation/NodeMapping.h>

namespace DEM::Anim
{

CStaticPose::CStaticPose(std::vector<Math::CTransformSRT>&& Transforms, PNodeMapping&& NodeMapping)
	: _Transforms(std::move(Transforms))
	, _NodeMapping(std::move(NodeMapping))
{
	n_assert_dbg(_NodeMapping->GetNodeCount() == _Transforms.size());
}
//---------------------------------------------------------------------

CStaticPose::~CStaticPose() = default;
//---------------------------------------------------------------------

void CStaticPose::Apply()
{
	//!!!must have output bound!
	//if (mapping.empty()) ... else ...

	//const auto OutputCount = GetTransformCount();
	//for (UPTR i = 0; i < OutputCount; ++i)
	//	SetTransform(i, _Transforms[i]);
}
//---------------------------------------------------------------------

}
