#include "AnimationLoaderANM.h"
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <acl/core/compressed_clip.h>

namespace Resources
{

const Core::CRTTI& CAnimationLoaderANM::GetResultType() const
{
	return DEM::Anim::CAnimationClip::RTTI;
}
//---------------------------------------------------------------------

Core::PObject CAnimationLoaderANM::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read(Magic) || Magic != 'ANIM') return nullptr;

	U32 FormatVersion;
	if (!Reader.Read(FormatVersion)) return nullptr;

	float Duration;
	if (!Reader.Read(Duration)) return nullptr;

	U32 SampleCount;
	if (!Reader.Read(SampleCount)) return nullptr;

	U16 NodeCount;
	if (!Reader.Read(NodeCount) || !NodeCount) return nullptr;

	// Children are always after the parent, as node mapping requires
	std::vector<DEM::Anim::CSkeletonInfo::CNodeInfo> NodeMapping(NodeCount);
	for (U16 i = 0; i < NodeCount; ++i)
	{
		if (!Reader.Read(NodeMapping[i].ParentIndex)) return nullptr;
		if (!Reader.Read(NodeMapping[i].ID)) return nullptr;
	}

	// Load locomotion info
	//???unique ptr to locomotion info structure?
	DEM::Anim::PBipedLocomotionInfo LocomotionInfo;
	U8 IsLocomotion;
	if (!Reader.Read(IsLocomotion)) return nullptr;
	if (IsLocomotion)
	{
		LocomotionInfo.reset(n_new(DEM::Anim::CBipedLocomotionInfo));
		if (!Reader.Read(LocomotionInfo->Speed)) return nullptr;
		if (!Reader.Read(LocomotionInfo->CycleStartFrame)) return nullptr;
		if (!Reader.Read(LocomotionInfo->LeftFootOnGroundFrame)) return nullptr;
		if (!Reader.Read(LocomotionInfo->RightFootOnGroundFrame)) return nullptr;

		U16 PhaseCount;
		if (!Reader.Read(PhaseCount)) return nullptr;

		LocomotionInfo->Phases.reset(n_new_array(float, PhaseCount));
		if (Stream->Read(LocomotionInfo->Phases.get(), PhaseCount * sizeof(float)) != PhaseCount * sizeof(float)) return nullptr;

		U16 PhaseTimeCount;
		if (!Reader.Read(PhaseTimeCount)) return nullptr;

		// Saved already sorted
		for (U16 i = 0; i < PhaseTimeCount; ++i)
		{
			float Phase, Time;
			if (!Reader.Read(Phase)) return nullptr;
			if (!Reader.Read(Time)) return nullptr;
			LocomotionInfo->PhaseNormalizedTimes.emplace_back(Phase, Time);
		}
	}

	// Skip padding
	U8 Padding;
	if (!Reader.Read(Padding)) return nullptr;
	Stream->Seek(Padding, IO::Seek_Current);

	U32 ClipDataSize;
	if (!Reader.Read(ClipDataSize) || !ClipDataSize) return nullptr;

	// Clip size is a part of clip data, so it must be read again as a part of the clip
	Stream->Seek(-4, IO::Seek_Current);

	void* pBuffer = n_malloc_aligned(ClipDataSize, alignof(acl::CompressedClip));

	//!!!clip data is aligned-16 relatively to the file beginning! can use MMF!
	if (!pBuffer || Stream->Read(pBuffer, ClipDataSize) != ClipDataSize)
	{
		SAFE_FREE_ALIGNED(pBuffer);
		return nullptr;
	}

	auto pClip = reinterpret_cast<acl::CompressedClip*>(pBuffer);
	if (!pClip->is_valid(true).empty())
	{
		SAFE_FREE_ALIGNED(pBuffer);
		return nullptr;
	}

	return n_new(DEM::Anim::CAnimationClip(pClip, Duration, SampleCount, new DEM::Anim::CSkeletonInfo(std::move(NodeMapping)), std::move(LocomotionInfo)));
}
//---------------------------------------------------------------------

}
