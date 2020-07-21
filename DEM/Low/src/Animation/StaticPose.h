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
	IPoseOutput*                     _pOutput = nullptr; //???PPoseOutput refcounted?
	std::vector<U16>                 _PortMapping;
	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Math::CTransformSRT>&& Transforms, PNodeMapping&& NodeMapping);
	~CStaticPose();

	void SetOutput(IPoseOutput& Output);
	void Apply();
};

}
