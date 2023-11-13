#pragma once
#include <Data/Ptr.h>
#include <acl/decompression/decompress.h>

// Transformation source that samples an animation clip

namespace DEM::Anim
{
class IPoseOutput;
class CPoseBuffer;
typedef Ptr<class CAnimationClip> PAnimationClip;
typedef std::unique_ptr<class CAnimationSampler> PAnimationSampler;
typedef std::unique_ptr<class CStaticPose> PStaticPose;
using CACLContext = acl::decompression_context<acl::default_transform_decompression_settings>;

class alignas(CACLContext) CAnimationSampler final
{
protected:

	CACLContext    _Context; // At offset 0
	PAnimationClip _Clip;

	//???store mask right here or separately? what bones (and maybe what components of SRT in them) are used.

public:

	DEM_ALLOCATE_ALIGNED(alignof(CAnimationSampler));

	CAnimationSampler();
	~CAnimationSampler();

	void  EvaluatePose(float Time, IPoseOutput& Output);
	void  EvaluatePose(float Time, CPoseBuffer& Output, U16* pMapping = nullptr);

	bool  SetClip(PAnimationClip Clip);
	auto* GetClip() const { return _Clip.Get(); }
};

}
