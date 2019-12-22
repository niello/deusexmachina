#include "StaticPose.h"
#include <Animation/AnimationBlender.h>
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

CStaticPose::CStaticPose(std::vector<Scene::CSceneNode*>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms)
	: _Nodes(std::move(Nodes))
	, _Transforms(std::move(Transforms))
{
	n_assert_dbg(_Nodes.size() == _Transforms.size());

	//!!!DBG TMP!
	for (auto& Node : _Nodes)
		_Outputs.push_back({ Node });
}
//---------------------------------------------------------------------

CStaticPose::~CStaticPose() = default;
//---------------------------------------------------------------------

void CStaticPose::SetBlending(PAnimationBlender Blender, U8 SourceIndex)
{
	if (Blender)
	{
		auto _Blender = std::move(Blender);
		auto _SourceIndex = SourceIndex;

		for (auto& Output : _Outputs)
			Output.BlenderPort = _Blender->GetOrCreateNodePort(*Output.Node);
	}
	else
	{
		for (auto& Output : _Outputs)
			Output.Node = Blender->GetPortNode(Output.BlenderPort);

		//_Blender.reset();
	}
}
//---------------------------------------------------------------------

void CStaticPose::Apply()
{
	for (UPTR i = 0; i < _Nodes.size(); ++i)
		_Nodes[i]->SetLocalTransform(_Transforms[i]);
}
//---------------------------------------------------------------------

}
