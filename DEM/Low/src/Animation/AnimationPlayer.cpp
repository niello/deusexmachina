#include "AnimationPlayer.h"
#include <Animation/AnimationClip.h>
#include <Animation/StaticPose.h>
#include <Scene/SceneNode.h>

namespace DEM::Anim
{

struct CScenePoseWriter : acl::OutputWriter
{
	CScenePoseWriter(Scene::CSceneNode** pNodes)
		: _pNodes(pNodes)
	{
		n_assert_dbg(_pNodes);
	}

	void write_bone_rotation(uint16_t bone_index, const acl::Quat_32& rotation)
	{
		_pNodes[bone_index]->SetRotation(quaternion(acl::quat_get_x(rotation), acl::quat_get_y(rotation), acl::quat_get_z(rotation), acl::quat_get_w(rotation)));
	}

	void write_bone_translation(uint16_t bone_index, const acl::Vector4_32& translation)
	{
		_pNodes[bone_index]->SetPosition(vector3(acl::vector_get_x(translation), acl::vector_get_y(translation), acl::vector_get_z(translation)));
	}

	void write_bone_scale(uint16_t bone_index, const acl::Vector4_32& scale)
	{
		_pNodes[bone_index]->SetScale(vector3(acl::vector_get_x(scale), acl::vector_get_y(scale), acl::vector_get_z(scale)));
	}

	Scene::CSceneNode** _pNodes;
};

struct CStaticPoseWriter : acl::OutputWriter
{
	CStaticPoseWriter(Math::CTransformSRT* pTransforms)
		: _pTransforms(pTransforms)
	{
		n_assert_dbg(pTransforms);
	}

	void write_bone_rotation(uint16_t bone_index, const acl::Quat_32& rotation)
	{
		_pTransforms[bone_index].Rotation =
			quaternion(acl::quat_get_x(rotation), acl::quat_get_y(rotation), acl::quat_get_z(rotation), acl::quat_get_w(rotation));
	}

	void write_bone_translation(uint16_t bone_index, const acl::Vector4_32& translation)
	{
		_pTransforms[bone_index].Translation = vector3(acl::vector_get_x(translation), acl::vector_get_y(translation), acl::vector_get_z(translation));
	}

	void write_bone_scale(uint16_t bone_index, const acl::Vector4_32& scale)
	{
		_pTransforms[bone_index].Scale = vector3(acl::vector_get_x(scale), acl::vector_get_y(scale), acl::vector_get_z(scale));
	}

	Math::CTransformSRT* _pTransforms;
};

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
	if (!Clip || !Clip->GetNodeCount() || Clip->GetDuration() <= 0.f) return false;

	const auto* pClip = Clip->GetACLClip();
	if (!pClip) return false;

	if (_Context.is_dirty(*pClip))
		_Context.initialize(*pClip);

	if (_Clip != Clip)
	{
		// TODO: can compare node mappings and skip if equal
		// if (_Nodes[0] = &RootNode)
		//	check all other node IDs and parent indices between clips
		// Can even store mapping hash inside a clip, compare root & mapping count & hash, then do full comparison!
		// It will be pretty common to reuse the same player with different clips on the same hierarchy.

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

	return true;
}
//---------------------------------------------------------------------

void CAnimationPlayer::Reset()
{
	_Clip = nullptr;
	_Nodes.clear();
	_CurrTime = 0.f;
	_Paused = true;
}
//---------------------------------------------------------------------

static inline float NormalizeAnimationCursor(float Time, float Duration, bool Loop)
{
	return Loop ? std::fmodf(Time, Duration) : std::clamp(Time, 0.f, Duration);
}
//---------------------------------------------------------------------

//???use per-bone decompression when many nodes are not bound? May be useful for LOD.
void CAnimationPlayer::Update(float dt)
{
	if (_Paused) return;

	_CurrTime = NormalizeAnimationCursor(_CurrTime + dt * _Speed, _Clip->GetDuration(), _Loop);

	_Context.seek(_CurrTime, acl::SampleRoundingPolicy::None);
	_Context.decompress_pose(CScenePoseWriter(_Nodes.data()));
}
//---------------------------------------------------------------------

PStaticPose CAnimationPlayer::BakePose(float Time)
{
	if (!_Clip || _Nodes.empty()) return nullptr;

	Time = NormalizeAnimationCursor(Time, _Clip->GetDuration(), _Loop);

	std::vector<Math::CTransformSRT> Tfms(_Nodes.size());

	_Context.seek(Time, acl::SampleRoundingPolicy::None);
	_Context.decompress_pose(CStaticPoseWriter(Tfms.data()));

	auto Nodes = _Nodes;
	return PStaticPose(n_new(CStaticPose(std::move(Nodes), std::move(Tfms))));
}
//---------------------------------------------------------------------

void CAnimationPlayer::SetCursor(float Time)
{
	if (!_Clip) return;
	_CurrTime = NormalizeAnimationCursor(Time, _Clip->GetDuration(), _Loop);
}
//---------------------------------------------------------------------

}
