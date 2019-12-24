#pragma once
#include <Animation/TransformSource.h>

// Transformation source that applies static transform to the node hierarchy

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{
typedef std::unique_ptr<class CStaticPose> PStaticPose;

class CStaticPose final : public CTransformSource
{
protected:

	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Scene::PSceneNode>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms);
	~CStaticPose();

	void Apply();
};

}
