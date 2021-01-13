#pragma once
#include <Data/Ptr.h>
#include <acl/algorithm/uniformly_sampled/decoder.h>

// Transformation source that samples an animation clip

namespace DEM::Anim
{
class IPoseOutput;
typedef Ptr<class CAnimationClip> PAnimationClip;
typedef std::unique_ptr<class CAnimationSampler> PAnimationSampler;
typedef std::unique_ptr<class CStaticPose> PStaticPose;
typedef acl::uniformly_sampled::DecompressionContext<acl::uniformly_sampled::DefaultDecompressionSettings> CACLContext;

class alignas(CACLContext) CAnimationSampler final
{
protected:

	CACLContext    _Context; // At offset 0
	PAnimationClip _Clip;

public:

	DEM_ALLOCATE_ALIGNED(alignof(CAnimationSampler));

	CAnimationSampler();
	~CAnimationSampler();

	void  EvaluatePose(float Time, IPoseOutput& Output);

	bool  SetClip(PAnimationClip Clip);
	auto* GetClip() const { return _Clip.Get(); }
};

}
