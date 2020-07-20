#include "StaticPose.h"
#include <Animation/AnimationBlender.h>
#include <Animation/NodeMapping.h>
#include <Animation/PoseOutput.h>

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

void CStaticPose::SetOutput(IPoseOutput& Output)
{
	_pOutput = &Output;
	_NodeMapping->Bind(Output, _PortMapping);
}
//---------------------------------------------------------------------

void CStaticPose::Apply()
{
	if (!_pOutput) return;

	const auto Count = _Transforms.size();
	if (_PortMapping.empty())
	{
		for (UPTR i = 0; i < Count; ++i)
			_pOutput->SetTransform(i, _Transforms[i]);
	}
	else
	{
		for (UPTR i = 0; i < Count; ++i)
			_pOutput->SetTransform(_PortMapping[i], _Transforms[i]);
	}
}
//---------------------------------------------------------------------

}
