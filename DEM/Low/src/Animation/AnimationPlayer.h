#pragma once
#include <Animation/TransformSource.h>
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
typedef std::unique_ptr<class CStaticPose> PStaticPose;
typedef acl::uniformly_sampled::DecompressionContext<acl::uniformly_sampled::DefaultDecompressionSettings> CACLContext;

class alignas(CACLContext) CAnimationPlayer final : public CTransformSource
{
protected:

	CACLContext                     _Context; // At offset 0
	PAnimationClip                  _Clip;

	float                           _Speed = 1.f;
	float                           _CurrTime = 0.f;
	bool                            _Paused = true;
	bool                            _Loop = false;

	void SetupChildNodes(U16 ParentIndex, Scene::CSceneNode& ParentNode);

public:

	DEM_ALLOCATE_ALIGNED(alignof(CAnimationPlayer));

	CAnimationPlayer();
	~CAnimationPlayer();

	bool  Initialize(Scene::CSceneNode& RootNode, PAnimationClip Clip, bool Loop = false, float Speed = 1.f);
	void  Reset();

	void  Update(float dt);

	PStaticPose BakePose(float Time);

	bool  Play() { _Paused = (!_Clip || _Nodes.empty()); return !_Paused; }
	void  Stop() { _Paused = true; _CurrTime = 0.f; }
	void  Pause() { _Paused = true; }

	void  SetSpeed(float Speed) { _Speed = Speed; }
	void  SetLooped(bool Loop) { _Loop = Loop; }
	void  SetCursor(float Time);

	auto* GetClip() const { return _Clip.Get(); }
	float GetSpeed() const { return _Speed; }
	float GetCursor() const { return _CurrTime; }
	bool  IsLooped() const { return _Loop; }
	bool  IsPlaying() const { return !_Paused; }
};

}
