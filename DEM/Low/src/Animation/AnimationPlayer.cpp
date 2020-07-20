#include "AnimationPlayer.h"
#include <Animation/AnimationClip.h>
#include <Animation/NodeMapping.h>
#include <Animation/PoseOutput.h>
#include <Animation/StaticPose.h>

namespace DEM::Anim
{

// Writes to IPoseOutput without port mapping (clip bone index IS the output port index).
// It is a common case, especially for playing single clip without blending.
struct COutputPoseWriter : acl::OutputWriter
{
	COutputPoseWriter(IPoseOutput* pOutput)
		: _pOutput(pOutput)
	{
		n_assert_dbg(_pOutput);
	}

	void write_bone_rotation(uint16_t bone_index, const acl::Quat_32& rotation)
	{
		_pOutput->SetRotation(bone_index, quaternion(acl::quat_get_x(rotation), acl::quat_get_y(rotation), acl::quat_get_z(rotation), acl::quat_get_w(rotation)));
	}

	void write_bone_translation(uint16_t bone_index, const acl::Vector4_32& translation)
	{
		_pOutput->SetTranslation(bone_index, vector3(acl::vector_get_x(translation), acl::vector_get_y(translation), acl::vector_get_z(translation)));
	}

	void write_bone_scale(uint16_t bone_index, const acl::Vector4_32& scale)
	{
		_pOutput->SetScale(bone_index, vector3(acl::vector_get_x(scale), acl::vector_get_y(scale), acl::vector_get_z(scale)));
	}

	bool skip_bone_rotation(uint16_t bone_index) const { return !_pOutput->IsRotationActive(bone_index); }
	bool skip_bone_translation(uint16_t bone_index) const { return !_pOutput->IsTranslationActive(bone_index); }
	bool skip_bone_scale(uint16_t bone_index) const { return !_pOutput->IsScalingActive(bone_index); }

	IPoseOutput* _pOutput;
};

// Writes to IPoseOutput with port mapping (clip bone index -> output port index)
struct COutputPoseWriterMapped : acl::OutputWriter
{
	COutputPoseWriterMapped(IPoseOutput* pOutput, const std::vector<U16>& PortMapping)
		: _pOutput(pOutput)
		, _PortMapping(PortMapping)
	{
		n_assert_dbg(_pOutput);
	}

	void write_bone_rotation(uint16_t bone_index, const acl::Quat_32& rotation)
	{
		_pOutput->SetRotation(_PortMapping[bone_index], quaternion(acl::quat_get_x(rotation), acl::quat_get_y(rotation), acl::quat_get_z(rotation), acl::quat_get_w(rotation)));
	}

	void write_bone_translation(uint16_t bone_index, const acl::Vector4_32& translation)
	{
		_pOutput->SetTranslation(_PortMapping[bone_index], vector3(acl::vector_get_x(translation), acl::vector_get_y(translation), acl::vector_get_z(translation)));
	}

	void write_bone_scale(uint16_t bone_index, const acl::Vector4_32& scale)
	{
		_pOutput->SetScale(_PortMapping[bone_index], vector3(acl::vector_get_x(scale), acl::vector_get_y(scale), acl::vector_get_z(scale)));
	}

	bool skip_bone_rotation(uint16_t bone_index) const { return !_pOutput->IsRotationActive(_PortMapping[bone_index]); }
	bool skip_bone_translation(uint16_t bone_index) const { return !_pOutput->IsTranslationActive(_PortMapping[bone_index]); }
	bool skip_bone_scale(uint16_t bone_index) const { return !_pOutput->IsScalingActive(_PortMapping[bone_index]); }

	IPoseOutput* _pOutput;
	const std::vector<U16>& _PortMapping;
};

// Writes to the plain array of CTransformSRT
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

/*
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
*/

bool CAnimationPlayer::Initialize(IPoseOutput& Output, PAnimationClip Clip, bool Loop, float Speed)
{
	if (!Clip || Clip->GetDuration() <= 0.f) return false;

	const auto* pClip = Clip->GetACLClip();
	if (!pClip) return false;

	if (_Context.is_dirty(*pClip))
		_Context.initialize(*pClip);

	_pOutput = &Output;

	if (_Clip != Clip)
	{
		// TODO: can compare node mappings of old and new clip and skip if equal.
		// Can even store mapping hash, compare root & mapping count & hash, then do full comparison.
		// It will be pretty common to reuse the same player with different clips on the same hierarchy.

		_Clip = std::move(Clip);
		_Clip->GetNodeMapping().Bind(Output, _PortMapping);
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
	_pOutput = nullptr;
	_CurrTime = 0.f;
	_Paused = true;
}
//---------------------------------------------------------------------

static inline float NormalizeClipCursor(float Time, float Duration, bool Loop)
{
	return Loop ? std::fmodf(Time, Duration) : std::clamp(Time, 0.f, Duration);
}
//---------------------------------------------------------------------

void CAnimationPlayer::Update(float dt)
{
	if (_Paused || !_pOutput) return;

	_CurrTime = NormalizeClipCursor(_CurrTime + dt * _Speed, _Clip->GetDuration(), _Loop);

	_Context.seek(_CurrTime, acl::SampleRoundingPolicy::None);
	if (_PortMapping.empty())
		_Context.decompress_pose(COutputPoseWriter(_pOutput));
	else
		_Context.decompress_pose(COutputPoseWriterMapped(_pOutput, _PortMapping));
}
//---------------------------------------------------------------------

PStaticPose CAnimationPlayer::BakePose(float Time)
{
	if (!_Clip || !_pOutput) return nullptr;

	Time = NormalizeClipCursor(Time, _Clip->GetDuration(), _Loop);

	std::vector<Math::CTransformSRT> Tfms(_Clip->GetNodeCount());

	_Context.seek(Time, acl::SampleRoundingPolicy::None);
	_Context.decompress_pose(CStaticPoseWriter(Tfms.data()));

	return PStaticPose(n_new(CStaticPose(std::move(Tfms), &_Clip->GetNodeMapping())));
}
//---------------------------------------------------------------------

void CAnimationPlayer::SetCursor(float Time)
{
	if (!_Clip) return;
	_CurrTime = NormalizeClipCursor(Time, _Clip->GetDuration(), _Loop);
}
//---------------------------------------------------------------------

}
