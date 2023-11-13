#include "AnimationSampler.h"
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/PoseOutput.h>
#include <Animation/PoseBuffer.h>

namespace DEM::Anim
{

// Writes bone transforms from ACL clip to IPoseOutput
struct COutputPoseWriter : public acl::track_writer
{
	IPoseOutput& _Output;

	COutputPoseWriter(IPoseOutput& Output) : _Output(Output) {}

	void write_bone_rotation(uint16_t bone_index, const rtm::quatf& rotation)
	{
		_Output.SetRotation(bone_index, quaternion(rtm::quat_get_x(rotation), rtm::quat_get_y(rotation), rtm::quat_get_z(rotation), rtm::quat_get_w(rotation)));
	}

	void write_bone_translation(uint16_t bone_index, const rtm::vector4f& translation)
	{
		_Output.SetTranslation(bone_index, vector3(rtm::vector_get_x(translation), rtm::vector_get_y(translation), rtm::vector_get_z(translation)));
	}

	void write_bone_scale(uint16_t bone_index, const rtm::vector4f& scale)
	{
		_Output.SetScale(bone_index, vector3(rtm::vector_get_x(scale), rtm::vector_get_y(scale), rtm::vector_get_z(scale)));
	}

	bool skip_bone_rotation(uint16_t bone_index) const { return !_Output.IsRotationActive(bone_index); }
	bool skip_bone_translation(uint16_t bone_index) const { return !_Output.IsTranslationActive(bone_index); }
	bool skip_bone_scale(uint16_t bone_index) const { return !_Output.IsScalingActive(bone_index); }
};

// Writes bone transforms from ACL clip to CPoseBuffer without mapping
// FIXME: pass quats and vectors by value using vectorcall, like ACL and RTM do?
struct CPoseBufferWriter : public acl::track_writer
{
	CPoseBuffer& _Output;

	CPoseBufferWriter(CPoseBuffer& Output) : _Output(Output) {}

	void write_bone_rotation(uint16_t bone_index, const rtm::quatf& rotation) { _Output[bone_index].rotation = rotation; }
	void write_bone_translation(uint16_t bone_index, const rtm::vector4f& translation) { _Output[bone_index].translation = translation; }
	void write_bone_scale(uint16_t bone_index, const rtm::vector4f& scale) { _Output[bone_index].scale = scale; }
};

// Writes bone transforms from ACL clip to CPoseBuffer with mapping
// FIXME: pass quats and vectors by value using vectorcall, like ACL and RTM do?
struct CMappedPoseBufferWriter : public acl::track_writer
{
	CPoseBuffer& _Output;
	U16*         _pMapping;

	CMappedPoseBufferWriter(CPoseBuffer& Output, U16* pMapping) : _Output(Output), _pMapping(pMapping) {}

	void write_bone_rotation(uint16_t bone_index, const rtm::quatf& rotation) { _Output[_pMapping[bone_index]].rotation = rotation; }
	void write_bone_translation(uint16_t bone_index, const rtm::vector4f& translation) { _Output[_pMapping[bone_index]].translation = translation; }
	void write_bone_scale(uint16_t bone_index, const rtm::vector4f& scale) { _Output[_pMapping[bone_index]].scale = scale; }

	bool skip_bone_rotation(uint16_t bone_index) const { return _pMapping[bone_index] == CSkeletonInfo::EmptyPort; }
	bool skip_bone_translation(uint16_t bone_index) const { return _pMapping[bone_index] == CSkeletonInfo::EmptyPort; }
	bool skip_bone_scale(uint16_t bone_index) const { return _pMapping[bone_index] == CSkeletonInfo::EmptyPort; }
};

CAnimationSampler::CAnimationSampler() = default;
CAnimationSampler::~CAnimationSampler() = default;

void CAnimationSampler::EvaluatePose(float Time, IPoseOutput& Output)
{
	if (!_Clip) return;
	_Context.seek(Time, acl::sample_rounding_policy::none);
	_Context.decompress_tracks(COutputPoseWriter(Output));
}
//---------------------------------------------------------------------

void CAnimationSampler::EvaluatePose(float Time, CPoseBuffer& Output, U16* pMapping)
{
	if (!_Clip) return;
	_Context.seek(Time, acl::sample_rounding_policy::none);
	if (pMapping)
		_Context.decompress_tracks(CMappedPoseBufferWriter(Output, pMapping));
	else
		_Context.decompress_tracks(CPoseBufferWriter(Output));
}
//---------------------------------------------------------------------

bool CAnimationSampler::SetClip(PAnimationClip Clip)
{
	if (!Clip || Clip->GetDuration() <= 0.f) return false;

	const auto* pACLClip = Clip->GetACLClip();
	if (!pACLClip) return false;

	if (_Context.is_bound_to(*pACLClip))
		_Context.initialize(*pACLClip);

	_Clip = std::move(Clip);

	return true;
}
//---------------------------------------------------------------------

}
