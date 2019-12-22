#pragma once
#include <Math/TransformSRT.h>
#include <vector>
#include <memory>

// Transformation source that applies static transform to the node hierarchy

namespace Scene
{
	class CSceneNode;
}

namespace DEM::Anim
{
typedef std::unique_ptr<class CStaticPose> PStaticPose;

class CStaticPose final
{
protected:

	std::vector<Scene::CSceneNode*> _Nodes; //???strong refs to nodes? or weak refs?
	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Scene::CSceneNode*>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms);
	~CStaticPose();
};

}
