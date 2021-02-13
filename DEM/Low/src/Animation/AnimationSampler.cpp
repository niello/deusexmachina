#include "AnimationSampler.h"
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/PoseOutput.h>
#include <Animation/PoseBuffer.h>

namespace DEM::Anim
{

// Writes bone transforms from ACL clip to IPoseOutput
struct COutputPoseWriter : public acl::OutputWriter
{
	IPoseOutput& _Output;

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
};

// Writes bone transforms from ACL clip to CPoseBuffer without mapping
// FIXME: use ACL transform in the DEM scene and animation?
struct CPoseBufferWriter : public acl::OutputWriter
{
	CPoseBuffer& _Output;

	CPoseBufferWriter(CPoseBuffer& Output) : _Output(Output) {}

	void write_bone_rotation(uint16_t bone_index, const acl::Quat_32& rotation)
	{
		_Output[bone_index].Rotation.set(acl::quat_get_x(rotation), acl::quat_get_y(rotation), acl::quat_get_z(rotation), acl::quat_get_w(rotation));
	}

	void write_bone_translation(uint16_t bone_index, const acl::Vector4_32& translation)
	{
		_Output[bone_index].Translation.set(acl::vector_get_x(translation), acl::vector_get_y(translation), acl::vector_get_z(translation));
	}

	void write_bone_scale(uint16_t bone_index, const acl::Vector4_32& scale)
	{
		_Output[bone_index].Scale.set(acl::vector_get_x(scale), acl::vector_get_y(scale), acl::vector_get_z(scale));
	}
};

// Writes bone transforms from ACL clip to CPoseBuffer with mapping
// FIXME: use ACL transform in the DEM scene and animation?
struct CMappedPoseBufferWriter : public acl::OutputWriter
{
	CPoseBuffer& _Output;
	U16* _pMapping;

	CMappedPoseBufferWriter(CPoseBuffer& Output, U16* pMapping) : _Output(Output), _pMapping(pMapping) {}

	void write_bone_rotation(uint16_t bone_index, const acl::Quat_32& rotation)
	{
		_Output[_pMapping[bone_index]].Rotation.set(acl::quat_get_x(rotation), acl::quat_get_y(rotation), acl::quat_get_z(rotation), acl::quat_get_w(rotation));
	}

	void write_bone_translation(uint16_t bone_index, const acl::Vector4_32& translation)
	{
		_Output[_pMapping[bone_index]].Translation.set(acl::vector_get_x(translation), acl::vector_get_y(translation), acl::vector_get_z(translation));
	}

	void write_bone_scale(uint16_t bone_index, const acl::Vector4_32& scale)
	{
		_Output[_pMapping[bone_index]].Scale.set(acl::vector_get_x(scale), acl::vector_get_y(scale), acl::vector_get_z(scale));
	}

	bool skip_bone_rotation(uint16_t bone_index) const { return _pMapping[bone_index] == CSkeletonInfo::EmptyPort; }
	bool skip_bone_translation(uint16_t bone_index) const { return _pMapping[bone_index] == CSkeletonInfo::EmptyPort; }
	bool skip_bone_scale(uint16_t bone_index) const { return _pMapping[bone_index] == CSkeletonInfo::EmptyPort; }
};

CAnimationSampler::CAnimationSampler() = default;
CAnimationSampler::~CAnimationSampler() = default;

void CAnimationSampler::EvaluatePose(float Time, IPoseOutput& Output)
{
	if (!_Clip) return;
	_Context.seek(Time, acl::SampleRoundingPolicy::None);
	_Context.decompress_pose(COutputPoseWriter(Output));
}
//---------------------------------------------------------------------

void CAnimationSampler::EvaluatePose(float Time, CPoseBuffer& Output, U16* pMapping)
{
	if (!_Clip) return;
	_Context.seek(Time, acl::SampleRoundingPolicy::None);
	if (pMapping)
		_Context.decompress_pose(CMappedPoseBufferWriter(Output, pMapping));
	else
		_Context.decompress_pose(CPoseBufferWriter(Output));
}
//---------------------------------------------------------------------

bool CAnimationSampler::SetClip(PAnimationClip Clip)
{
	if (!Clip || Clip->GetDuration() <= 0.f) return false;

	const auto* pACLClip = Clip->GetACLClip();
	if (!pACLClip) return false;

	if (_Context.is_dirty(*pACLClip))
		_Context.initialize(*pACLClip);

	_Clip = std::move(Clip);

	return true;
}
//---------------------------------------------------------------------

}
