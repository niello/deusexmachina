#include "AnimationPlayer.h"
#include <Animation/AnimationClip.h>
#include <Animation/NodeMapping.h>
#include <Animation/PoseOutput.h>
#include <Animation/StaticPose.h>

namespace DEM::Anim
{

// Writes bone transforms from ACL clip to IPoseOutput
struct COutputPoseWriter : acl::OutputWriter
{
	COutputPoseWriter(IPoseOutput& Output) : _Output(Output) {}

	void write_bone_rotation(uint16_t bone_index, const acl::Quat_32& rotation)
	{
		_Output.SetRotation(bone_index, quaternion(acl::quat_get_x(rotation), acl::quat_get_y(rotation), acl::quat_get_z(rotation), acl::quat_get_w(rotation)));
	}

	void write_bone_translation(uint16_t bone_index, const acl::Vector4_32& translation)
	{
		_Output.SetTranslation(bone_index, vector3(acl::vector_get_x(translation), acl::vector_get_y(translation), acl::vector_get_z(translation)));
	}

	void write_bone_scale(uint16_t bone_index, const acl::Vector4_32& scale)
	{
		_Output.SetScale(bone_index, vector3(acl::vector_get_x(scale), acl::vector_get_y(scale), acl::vector_get_z(scale)));
	}

	bool skip_bone_rotation(uint16_t bone_index) const { return !_Output.IsRotationActive(bone_index); }
	bool skip_bone_translation(uint16_t bone_index) const { return !_Output.IsTranslationActive(bone_index); }
	bool skip_bone_scale(uint16_t bone_index) const { return !_Output.IsScalingActive(bone_index); }

	IPoseOutput& _Output;
};

// TODO: use CPoseRecorder : public IPoseOutput with COutputPoseWriter instead of this!
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

static inline float NormalizeClipCursor(float Time, float Duration, bool Loop)
{
	return Loop ? std::fmodf(Time, Duration) : std::clamp(Time, 0.f, Duration);
}
//---------------------------------------------------------------------

void CAnimationPlayer::Update(float dt, IPoseOutput& Output)
{
	if (_Paused) return;

	_CurrTime = NormalizeClipCursor(_CurrTime + dt * _Speed, _Clip->GetDuration(), _Loop);

	_Context.seek(_CurrTime, acl::SampleRoundingPolicy::None);
	_Context.decompress_pose(COutputPoseWriter(Output));
}
//---------------------------------------------------------------------

PStaticPose CAnimationPlayer::BakePose(float Time)
{
	if (!_Clip) return nullptr;

	Time = NormalizeClipCursor(Time, _Clip->GetDuration(), _Loop);

	//!!!use CPoseRecorder!
	std::vector<Math::CTransformSRT> Tfms(_Clip->GetNodeCount());

	_Context.seek(Time, acl::SampleRoundingPolicy::None);
	_Context.decompress_pose(CStaticPoseWriter(Tfms.data()));

	return PStaticPose(n_new(CStaticPose(std::move(Tfms), &_Clip->GetNodeMapping())));
}
//---------------------------------------------------------------------

bool CAnimationPlayer::SetClip(PAnimationClip Clip)
{
	if (!Clip || Clip->GetDuration() <= 0.f) return false;

	const auto* pACLClip = Clip->GetACLClip();
	if (!pACLClip) return false;

	if (_Context.is_dirty(*pACLClip))
		_Context.initialize(*pACLClip);

	_Clip = std::move(Clip);

	_CurrTime = 0.f;
	_Paused = true;

	return true;
}
//---------------------------------------------------------------------

void CAnimationPlayer::SetCursor(float Time)
{
	if (!_Clip) return;
	_CurrTime = NormalizeClipCursor(Time, _Clip->GetDuration(), _Loop);
}
//---------------------------------------------------------------------

void CAnimationPlayer::Reset()
{
	_Clip = nullptr;
	_CurrTime = 0.f;
	_Paused = true;
}
//---------------------------------------------------------------------

}
