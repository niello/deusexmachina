#include "StaticPose.h"
#include <Animation/SceneNodeMapping.h>
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{

CStaticPose::CStaticPose(std::vector<Math::CTransformSRT>&& Transforms, PSceneNodeMapping&& NodeMapping)
	: _Transforms(std::move(Transforms))
	, _NodeMapping(std::move(NodeMapping))
{
	n_assert_dbg(_NodeMapping->GetNodeCount() == _Transforms.size());
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
