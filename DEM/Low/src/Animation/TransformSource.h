#pragma once
#include <Data/Ptr.h>
#include <Math/TransformSRT.h>
#include <vector>
#include <memory>

// Transformation source that applies static transform to the node hierarchy

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{
typedef std::unique_ptr<class CStaticPose> PStaticPose;
typedef Ptr<class CAnimationBlender> PAnimationBlender;

class CStaticPose final
{
protected:

	PAnimationBlender                _Blender;
	U8                               _SourceIndex = 0;

	union
	{
		Scene::PSceneNode*           _pNodes = nullptr; // Used if _Blender is nullptr
		U16*                         _pBlenderPorts;    // Used if _Blender is set
	};

	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Scene::PSceneNode>&& Nodes, std::vector<Math::CTransformSRT>&& Transforms);
	~CStaticPose();

	void SetBlending(PAnimationBlender Blender, U8 SourceIndex);

	void Apply();
};

}
