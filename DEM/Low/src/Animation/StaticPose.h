#pragma once
#include <Data/Ptr.h>
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
typedef Ptr<class CAnimationBlender> PAnimationBlender;

class CStaticPose final
{
protected:

	union UOutput
	{
		Scene::CSceneNode* Node; //???strong refs to nodes? or weak refs?
		U16                BlenderPort;
	};
	std::vector<UOutput> _Outputs;

	std::vector<Scene::CSceneNode*> _Nodes; //???strong refs to nodes? or weak refs?
	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Scene::CSceneNode*>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms);
	~CStaticPose();

	void SetBlending(PAnimationBlender Blender, U8 SourceIndex);

	void Apply();
};

}
