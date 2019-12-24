#include "StaticPose.h"
#include <Animation/AnimationBlender.h>
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

CStaticPose::CStaticPose(std::vector<Scene::PSceneNode>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms)
	: _Transforms(std::move(Transforms))
{
	_Nodes = std::move(Nodes);
	n_assert_dbg(_Nodes.size() == _Transforms.size());
}
//---------------------------------------------------------------------

CStaticPose::~CStaticPose() = default;
//---------------------------------------------------------------------

void CStaticPose::Apply()
{
	const auto OutputCount = GetTransformCount();
	for (UPTR i = 0; i < OutputCount; ++i)
		SetTransform(i, _Transforms[i]);

	// TODO: profile if performance problems arise
/*
	if (_BlendInfo)
	{
		const auto SourceIndex = _BlendInfo->SourceIndex;
		const auto OutputCount = _BlendInfo->Ports.size();
		for (UPTR i = 0; i < OutputCount; ++i)
			_BlendInfo->Blender->SetTransform(SourceIndex, _BlendInfo->Ports[i], _Transforms[i]);
	}
	else
	{
		const auto OutputCount = _Nodes.size();
		for (UPTR i = 0; i < OutputCount; ++i)
			_Nodes[i]->SetLocalTransform(_Transforms[i]);
	}
*/
}
//---------------------------------------------------------------------

}
