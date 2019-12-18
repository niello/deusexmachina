#pragma once
#include <acl/algorithm/uniformly_sampled/decoder.h>

// Transformation source that samples an animation clip into the node hierarchy

namespace DEM::Anim
{
class CAnimationClip;

class CAnimationPlayer
{
protected:

	acl::uniformly_sampled::DecompressionContext<acl::uniformly_sampled::DefaultDecompressionSettings> _Context;

	//!!!hold clip!

public:

	//???split into SetClip and Play/Stop/Pause/Rewind etc?
	//!!!need Weight! possibly into Update/Apply/Advance or whatever name sampling takes!

	bool Play(const CAnimationClip& Clip, float Speed = 1.f, bool Loop = false);
};

}
