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
typedef std::unique_ptr<class CAnimationPlayer> PAnimationPlayer;
typedef acl::uniformly_sampled::DecompressionContext<acl::uniformly_sampled::DefaultDecompressionSettings> CACLContext;

class alignas(CACLContext) CAnimationPlayer final
{
protected:

	CACLContext                     _Context; // At offset 0
	PAnimationClip                  _Clip;
	std::vector<Scene::CSceneNode*> _Nodes; //???strong refs to nodes? or weak refs?

	float                           _Speed = 1.f;
	float                           _CurrTime = 0.f;
	bool                            _Paused = true;
	bool                            _Loop = false;

	void SetupChildNodes(U16 ParentIndex, Scene::CSceneNode& ParentNode);

public:

	DEM_ALLOCATE_ALIGNED(alignof(CAnimationPlayer));

	CAnimationPlayer();
	~CAnimationPlayer();

	//???split into SetClip and Play/Stop/Pause/Rewind etc?
	//!!!need Weight! possibly into Update/Apply/Advance or whatever name sampling takes!

	bool Initialize(Scene::CSceneNode& RootNode, PAnimationClip Clip, float Speed = 1.f, bool Loop = false);
	//Reset()

	void  Play() { _Paused = false; }
	void  Stop() { _Paused = true; _CurrTime = 0.f; }
	void  Pause() { _Paused = true; }
	void  SetCursor(float Time);
	float GetCursor() const { return _CurrTime; }
};

}
