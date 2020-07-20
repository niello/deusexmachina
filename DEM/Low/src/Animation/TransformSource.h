#pragma once
#include <Animation/AnimationBlender.h>
#include <Scene/SceneNode.h>

// Transformation source for scene node animation. Supports direct writing and blending.
// One source can't be reused for animating different node hierarchies at the same time.

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{

class CTransformSource
{
protected:

	struct CBlendInfo
	{
		PAnimationBlender Blender;
		std::vector<U16>  Ports;
		U8                SourceIndex;
	};

	std::vector<Scene::PSceneNode> _Nodes;
	std::unique_ptr<CBlendInfo>    _BlendInfo; // If empty, direct writing will be performed

public:

	void SetBlending(PAnimationBlender Blender, U8 SourceIndex);
};

}
