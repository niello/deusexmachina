#include "AnimationPlayer.h"
#include <Animation/AnimationClip.h>

namespace DEM::Anim
{
CAnimationPlayer::CAnimationPlayer() = default;
CAnimationPlayer::~CAnimationPlayer() = default;

bool CAnimationPlayer::Initialize(const Scene::CSceneNode& RootNode, PAnimationClip Clip, float Speed, bool Loop)
{
	if (!Clip) return false;

	const auto* pClip = Clip->GetACLClip();
	if (pClip) return false;

	if (_Context.is_dirty(*pClip))
	{
		_Context.initialize(*pClip);
		_Context.seek(0.f, acl::SampleRoundingPolicy::None);
	}

	if (_Clip != Clip)
	{
		// map nodes

		_Clip = Clip;
	}

	_CurrTime = 0.f;
	_Paused = true;

	/*
	context.decompress_bone(bone_index, &rotation, &translation, &scale);

	Transform_32* transforms = new Transform_32[num_bones];
	DefaultOutputWriter pose_writer(transforms, num_bones);
	context.decompress_pose(pose_writer);
	*/

	return false;
}
//---------------------------------------------------------------------

}
