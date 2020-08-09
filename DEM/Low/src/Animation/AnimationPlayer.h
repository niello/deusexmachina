#pragma once
#include <Data/Ptr.h>
#include <acl/algorithm/uniformly_sampled/decoder.h>

// Transformation source that samples an animation clip

namespace DEM::Anim
{
class IPoseOutput;
typedef Ptr<class CAnimationClip> PAnimationClip;
typedef std::unique_ptr<class CAnimationPlayer> PAnimationPlayer;
typedef std::unique_ptr<class CStaticPose> PStaticPose;
typedef acl::uniformly_sampled::DecompressionContext<acl::uniformly_sampled::DefaultDecompressionSettings> CACLContext;

class alignas(CACLContext) CAnimationPlayer final
{
protected:

	CACLContext      _Context; // At offset 0
	PAnimationClip   _Clip;

	float            _Speed = 1.f;
	float            _CurrTime = 0.f;
	bool             _Paused = true;
	bool             _Loop = false;

public:

	DEM_ALLOCATE_ALIGNED(alignof(CAnimationPlayer));

	CAnimationPlayer();
	~CAnimationPlayer();

	void        Apply(IPoseOutput& Output);
	PStaticPose BakePose(float Time);

	bool  Play() { _Paused = !_Clip; return !_Paused; }
	void  Stop() { _Paused = true; _CurrTime = 0.f; }
	void  Pause() { _Paused = true; }

	bool  SetClip(PAnimationClip Clip);
	void  SetSpeed(float Speed) { _Speed = Speed; }
	void  SetLooped(bool Loop) { _Loop = Loop; }
	void  SetCursor(float Time);
	void  Reset();

	auto* GetClip() const { return _Clip.Get(); }
	float GetSpeed() const { return _Speed; }
	float GetCursor() const { return _CurrTime; }
	bool  IsLooped() const { return _Loop; }
	bool  IsPlaying() const { return !_Paused; }
};

}
