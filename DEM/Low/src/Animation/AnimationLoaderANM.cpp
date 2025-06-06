#include "AnimationLoaderANM.h"
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/Timeline/EventClip.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Data/DataArray.h>
#include <acl/core/compressed_tracks.h>

namespace Resources
{
static const CStrID sidID("ID");
static const CStrID sidTime("Time");

const DEM::Core::CRTTI& CAnimationLoaderANM::GetResultType() const
{
	return DEM::Anim::CAnimationClip::RTTI;
}
//---------------------------------------------------------------------

DEM::Core::PObject CAnimationLoaderANM::CreateResource(CStrID UID)
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

	void* pBuffer = n_malloc_aligned(ClipDataSize, alignof(acl::compressed_tracks));
	if (!pBuffer) return nullptr;

	// Clip size is a part of clip data, so it must be put back to first 4 bytes of buffer
	//!!!clip data is aligned-16 relatively to the file beginning! can use MMF with memcpy?!
	*static_cast<U32*>(pBuffer) = ClipDataSize;
	if (Stream->Read(static_cast<uint8_t*>(pBuffer) + 4, ClipDataSize - 4) != ClipDataSize - 4)
	{
		SAFE_FREE_ALIGNED(pBuffer);
		return nullptr;
	}

	auto pTracks = acl::make_compressed_tracks(pBuffer);
#if _DEBUG
	constexpr bool ValidateHash = true;
#else
	constexpr bool ValidateHash = false;
#endif
	if (!pTracks || pTracks->is_valid(ValidateHash).any())
	{
		SAFE_FREE_ALIGNED(pBuffer);
		return nullptr;
	}

	DEM::Anim::PEventClip EventClip;
	U32 EventSig;
	if (Reader.Read(EventSig) && EventSig == 'EVNT')
	{
		EventClip = std::make_unique<DEM::Anim::CEventClip>();

		Data::CDataArray Events;
		Reader.Read(Events);
		for (const auto& Event : Events)
		{
			auto* ppParams = Event.As<Data::PParams>();
			if (!ppParams || !*ppParams) continue;
			Data::PParams Params = *ppParams;

			const CStrID ID = Params->Get<CStrID>(sidID, CStrID::Empty);
			if (!ID) continue;

			const float Time = std::clamp(Params->Get<float>(sidTime, 0.f), 0.f, Duration);

			Params->Remove(sidID);
			Params->Remove(sidTime);
			if (!Params->GetCount())
				Params = nullptr;

			EventClip->AddEvent(Time, ID, Params);
		}
	}

	return n_new(DEM::Anim::CAnimationClip(pTracks, Duration, SampleCount, new DEM::Anim::CSkeletonInfo(std::move(NodeMapping)), std::move(EventClip), std::move(LocomotionInfo)));
}
//---------------------------------------------------------------------

}
