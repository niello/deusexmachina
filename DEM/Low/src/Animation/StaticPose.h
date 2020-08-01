#pragma once
#include <Data/Ptr.h>
#include <Math/TransformSRT.h>

// Transformation source that applies static transform

// NB: it is data (like CAnimationClip) and a player (like CAnimationPlayer) at the same time

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{
using PStaticPose = std::unique_ptr<class CStaticPose>;
using PNodeMapping = Ptr<class CNodeMapping>;
class IPoseOutput;

class CStaticPose final
{
protected:

	PNodeMapping                     _NodeMapping;
	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Math::CTransformSRT>&& Transforms, PNodeMapping&& NodeMapping);
	~CStaticPose();

	void Apply(IPoseOutput& Output);

	CNodeMapping& GetNodeMapping() const { return *_NodeMapping; } // non-const to create intrusive strong refs
};

}
