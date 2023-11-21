#pragma once
#include <Data/Ptr.h>
#include <rtm/qvvf.h>

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

	PSkeletonInfo          _SkeletonInfo;
	std::vector<rtm::qvvf> _Transforms;

public:

	CStaticPose(std::vector<rtm::qvvf>&& Transforms, PSkeletonInfo&& SkeletonInfo);
	~CStaticPose();

	void Apply(IPoseOutput& Output);

	CSkeletonInfo& GetSkeletonInfo() const { return *_SkeletonInfo; } // non-const to create intrusive strong refs
};

}
