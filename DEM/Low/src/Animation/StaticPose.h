#pragma once
#include <Animation/TransformSource.h>

// Transformation source that applies static transform

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DEM::Anim
{
using PStaticPose = std::unique_ptr<class CStaticPose>;
using PNodeMapping = Ptr<class CNodeMapping>;

class CStaticPose final
{
protected:

	std::vector<Math::CTransformSRT> _Transforms;
	PNodeMapping                     _NodeMapping;

public:

	CStaticPose(std::vector<Math::CTransformSRT>&& Transforms, PNodeMapping&& NodeMapping);
	~CStaticPose();

	void Apply();
};

}
