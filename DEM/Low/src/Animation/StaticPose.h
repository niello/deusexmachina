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
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
class IPoseOutput;

class CStaticPose final
{
protected:

	PSkeletonInfo                    _SkeletonInfo;
	std::vector<Math::CTransformSRT> _Transforms;

public:

	CStaticPose(std::vector<Math::CTransformSRT>&& Transforms, PSkeletonInfo&& SkeletonInfo);
	~CStaticPose();

	void Apply(IPoseOutput& Output);

	CSkeletonInfo& GetSkeletonInfo() const { return *_SkeletonInfo; } // non-const to create intrusive strong refs
};

}
