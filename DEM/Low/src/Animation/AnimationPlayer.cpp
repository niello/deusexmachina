#include "AnimationPlayer.h"
#include <Animation/AnimationClip.h>
#include <Scene/SceneNode.h>

namespace DEM::Anim
{
CAnimationPlayer::CAnimationPlayer() = default;
CAnimationPlayer::~CAnimationPlayer() = default;

void CAnimationPlayer::SetupChildNodes(U16 ParentIndex, Scene::CSceneNode& ParentNode)
{
	const auto NodeCount = _Clip->GetNodeCount();
	for (UPTR i = ParentIndex + 1; i < NodeCount; ++i)
	{
		auto& pBoneNode = _Nodes[i];
		if (pBoneNode) continue;

		const auto& NodeInfo = _Clip->GetNodeInfo(i);
		if (NodeInfo.ParentIndex != ParentIndex) continue;

		if (pBoneNode = ParentNode.GetChild(NodeInfo.ID))
			SetupChildNodes(i, *pBoneNode);
	}
}
//---------------------------------------------------------------------

bool CAnimationPlayer::Initialize(Scene::CSceneNode& RootNode, PAnimationClip Clip, float Speed, bool Loop)
{
	if (!Clip || !Clip->GetNodeCount()) return false;

	const auto* pClip = Clip->GetACLClip();
	if (!pClip) return false;

	if (_Context.is_dirty(*pClip))
		_Context.initialize(*pClip);

	if (_Clip != Clip)
	{
		_Nodes.resize(Clip->GetNodeCount());
		_Clip = std::move(Clip);

		// DEM animation format forces the root to be at position 0
		_Nodes[0] = &RootNode;
		SetupChildNodes(0, RootNode);
	}

	_CurrTime = 0.f;
	_Paused = true;

	/*
	_Context.seek(0.f, acl::SampleRoundingPolicy::None);

	context.decompress_bone(bone_index, &rotation, &translation, &scale);

	Transform_32* transforms = new Transform_32[num_bones];
	DefaultOutputWriter pose_writer(transforms, num_bones);
	context.decompress_pose(pose_writer);
	*/

	return false;
}
//---------------------------------------------------------------------

}
