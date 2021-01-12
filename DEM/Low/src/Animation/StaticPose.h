#pragma once
#include <Data/Ptr.h>
#include <Math/TransformSRT.h>

// Transformation source that applies static transform

// NB: it is data (like CAnimationClip) and a player (like CAnimationSampler) at the same time

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{
using PStaticPose = std::unique_ptr<class CStaticPose>;
using PSceneNodeMapping = Ptr<class CSceneNodeMapping>;
class IPoseOutput;

class CStaticPose final
{
protected:

	PSceneNodeMapping                _NodeMapping;
	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Math::CTransformSRT>&& Transforms, PSceneNodeMapping&& NodeMapping);
	~CStaticPose();

	void Apply(IPoseOutput& Output);

	CSceneNodeMapping& GetNodeMapping() const { return *_NodeMapping; } // non-const to create intrusive strong refs
};

}
