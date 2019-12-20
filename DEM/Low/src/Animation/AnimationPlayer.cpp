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

bool CAnimationPlayer::Initialize(Scene::CSceneNode& RootNode, PAnimationClip Clip, bool Loop, float Speed)
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

	_Speed = Speed;
	_Loop = Loop;

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

void CAnimationPlayer::Update(float dt)
{
	if (!_Clip || _Paused) return;

	SetCursor(_CurrTime + dt * _Speed);

	_Context.seek(_CurrTime, acl::SampleRoundingPolicy::None);

	// FIXME: use pose decompression instead!
	for (UPTR i = 0; i < _Nodes.size(); ++i)
	{
		if (!_Nodes[i]) continue;

		acl::Quat_32 R;
		acl::Vector4_32 T;
		acl::Vector4_32 S;
		_Context.decompress_bone(i, &R, &T, &S);

		_Nodes[i]->SetRotation(quaternion(acl::quat_get_x(R), acl::quat_get_y(R), acl::quat_get_z(R), acl::quat_get_w(R)));
		_Nodes[i]->SetPosition(vector3(acl::vector_get_x(T), acl::vector_get_y(T), acl::vector_get_z(T)));
		_Nodes[i]->SetScale(vector3(acl::vector_get_x(S), acl::vector_get_y(S), acl::vector_get_z(S)));
	}
}
//---------------------------------------------------------------------

void CAnimationPlayer::SetCursor(float Time)
{
	if (!_Clip) return;
	if (_Loop) _CurrTime = std::fmodf(Time, _Clip->GetDuration());
	else _CurrTime = std::clamp(Time, 0.f, _Clip->GetDuration());
}
//---------------------------------------------------------------------

}
