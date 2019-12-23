#pragma once
#include <Data/Ptr.h>
#include <vector>
#include <memory>

// Transformation source for scene node animation. Supports direct writing and blending.

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{
typedef Ptr<class CAnimationBlender> PAnimationBlender;

class CTransformSource final
{
protected:

	//union UOutput
	//{
	//	Scene::PSceneNode Node;
	//	U16               BlenderPort;
	//};

	PAnimationBlender      _Blender;
	U8                     _SourceIndex = 0;

	////////////////////////////
	U16                    _NodeCount = 0;
	union
	{
		Scene::PSceneNode* _pNodes = nullptr; // Used if _Blender is nullptr
		U16*               _pBlenderPorts;    // Used if _Blender is set
	};

public:

	void SetBlending(PAnimationBlender Blender, U8 SourceIndex);
};

}
