#include "StaticPose.h"
#include <Animation/AnimationBlender.h>
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

CStaticPose::CStaticPose(std::vector<Scene::PSceneNode>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms)
	: _Nodes(std::move(Nodes))
	, _Transforms(std::move(Transforms))
{
	n_assert_dbg(_Nodes.size() == _Transforms.size());
}
//---------------------------------------------------------------------

CStaticPose::~CStaticPose() = default;
//---------------------------------------------------------------------

void CStaticPose::SetBlending(PAnimationBlender Blender, U8 SourceIndex)
{
	if (Blender)
	{
		_Blender = std::move(Blender);
		_SourceIndex = SourceIndex;

		//for (auto& Output : _Outputs)
		//	Output.BlenderPort = _Blender->GetOrCreateNodePort(*Output.Node);
	}
	else
	{
		//for (auto& Output : _Outputs)
		//	Output.Node = _Blender->GetPortNode(Output.BlenderPort);

		_Blender = nullptr;
	}
}
//---------------------------------------------------------------------

void CStaticPose::Apply()
{
	if (_Blender)
	{
		//for (UPTR i = 0; i < _Outputs.size(); ++i)
		//	_Blender->SetTransform(_SourceIndex, _Outputs[i].BlenderPort, _Transforms[i]);
	}
	else
	{
		//for (UPTR i = 0; i < _Outputs.size(); ++i)
		//	_Outputs[i].Node->SetLocalTransform(_Transforms[i]);
	}
}
//---------------------------------------------------------------------

}
