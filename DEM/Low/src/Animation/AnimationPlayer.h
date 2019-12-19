#pragma once
#include <Data/Ptr.h>
#include <acl/algorithm/uniformly_sampled/decoder.h>

// Transformation source that samples an animation clip into the node hierarchy

namespace Scene
{
	class CSceneNode;
}

namespace DEM::Anim
{
typedef Ptr<class CAnimationClip> PAnimationClip;

class CAnimationPlayer final
{
protected:

	acl::uniformly_sampled::DecompressionContext<acl::uniformly_sampled::DefaultDecompressionSettings> _Context;

	PAnimationClip _Clip;
	//???strong refs to nodes? or weak refs?

	float _Speed = 1.f;
	float _CurrTime = 0.f;
	bool _Paused = true;
	bool _Loop = false;

public:

	CAnimationPlayer();
	~CAnimationPlayer();

	//???split into SetClip and Play/Stop/Pause/Rewind etc?
	//!!!need Weight! possibly into Update/Apply/Advance or whatever name sampling takes!

	bool Initialize(const Scene::CSceneNode& RootNode, PAnimationClip Clip, float Speed = 1.f, bool Loop = false);
	//Reset()

	bool Play();
};

}
