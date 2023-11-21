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

	void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation) { _Output.SetRotation(track_index, rotation); }
	void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation) { _Output.SetTranslation(track_index, translation); }
	void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale) { _Output.SetScale(track_index, scale); }

	bool skip_track_rotation(uint32_t track_index) const { return !_Output.IsRotationActive(track_index); }
	bool skip_track_translation(uint32_t track_index) const { return !_Output.IsTranslationActive(track_index); }
	bool skip_track_scale(uint32_t track_index) const { return !_Output.IsScalingActive(track_index); }
};

// Writes bone transforms from ACL clip to CPoseBuffer without mapping
struct CPoseBufferWriter : public acl::track_writer
{
	CPoseBuffer& _Output;

	CPoseBufferWriter(CPoseBuffer& Output) : _Output(Output) {}

	void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation) { _Output[track_index].rotation = rotation; }
	void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation) { _Output[track_index].translation = translation; }
	void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale) { _Output[track_index].scale = scale; }
};

// Writes bone transforms from ACL clip to CPoseBuffer with mapping
struct CMappedPoseBufferWriter : public acl::track_writer
{
	CPoseBuffer& _Output;
	U16*         _pMapping;

	CMappedPoseBufferWriter(CPoseBuffer& Output, U16* pMapping) : _Output(Output), _pMapping(pMapping) {}

	void RTM_SIMD_CALL write_rotation(uint32_t track_index, rtm::quatf_arg0 rotation) { _Output[_pMapping[track_index]].rotation = rotation; }
	void RTM_SIMD_CALL write_translation(uint32_t track_index, rtm::vector4f_arg0 translation) { _Output[_pMapping[track_index]].translation = translation; }
	void RTM_SIMD_CALL write_scale(uint32_t track_index, rtm::vector4f_arg0 scale) { _Output[_pMapping[track_index]].scale = scale; }

	bool skip_track_rotation(uint32_t track_index) const { return _pMapping[track_index] == CSkeletonInfo::EmptyPort; }
	bool skip_track_translation(uint32_t track_index) const { return _pMapping[track_index] == CSkeletonInfo::EmptyPort; }
	bool skip_track_scale(uint32_t track_index) const { return _pMapping[track_index] == CSkeletonInfo::EmptyPort; }
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

	if (!_Context.is_bound_to(*pACLClip))
		_Context.initialize(*pACLClip);

	_Clip = std::move(Clip);

	return true;
}
//---------------------------------------------------------------------

}
