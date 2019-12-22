#include "StaticPose.h"
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

CStaticPose::CStaticPose(std::vector<Scene::CSceneNode*>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms)
	: _Nodes(std::move(Nodes))
	, _Transforms(std::move(Transforms))
{
	n_assert_dbg(_Nodes.size() == _Transforms.size());
}
//---------------------------------------------------------------------

CStaticPose::~CStaticPose() = default;
//---------------------------------------------------------------------

void CStaticPose::Apply()
{
	for (UPTR i = 0; i < _Nodes.size(); ++i)
		_Nodes[i]->SetLocalTransform(_Transforms[i]);
}
//---------------------------------------------------------------------

}
