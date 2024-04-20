#include "PoseTrack.h"
#include <Animation/Timeline/PoseClipBase.h>
#include <Animation/PoseOutput.h>
#include <Animation/SkeletonInfo.h>

namespace DEM::Anim
{
CPoseTrack::CPoseTrack(CStrID Name) : CTimelineTrack(Name) {}
CPoseTrack::~CPoseTrack() = default;

// FIXME: this logic is the same for any clip/track type
void CPoseTrack::AddClip(PPoseClipBase&& Clip, float StartTime, float Duration /*, overlap resolve mode*/)
{
	if (!Clip || Duration <= 0.f) return;
	auto It = std::lower_bound(_Clips.begin(), _Clips.end(), StartTime, [](const CClip& Clip, float Time) { return Clip.EndTime < Time; });
	_Clips.insert(It, CClip{ std::move(Clip), StartTime, StartTime + Duration });
}
//---------------------------------------------------------------------

//!!!recurse through blenders, handle mapped outputs for leaf sources (like in loading now).
void CPoseTrack::RefreshSkeletonInfo()
{
	//???do right in AddClip?
	_SkeletonInfo = nullptr;
	for (auto& [Clip, StartTime, EndTime] : _Clips)
		Clip->GatherSkeletonInfo(_SkeletonInfo);
}
//---------------------------------------------------------------------

void CPoseTrack::SetOutput(PPoseOutput&& Output)
{
	_Output = std::move(Output); // In cpp because requires an IPoseOutput definition
}
//---------------------------------------------------------------------

PTimelineTrack CPoseTrack::Clone() const
{
	PPoseTrack NewTrack = n_new(CPoseTrack(_Name));
	for (const auto& ClipData : _Clips)
	{
		CClip NewClipData;
		NewClipData.Clip = ClipData.Clip ? ClipData.Clip->Clone() : nullptr;
		NewClipData.StartTime = ClipData.StartTime;
		NewClipData.EndTime = ClipData.EndTime;
		NewTrack->_Clips.push_back(std::move(NewClipData));
	}

	NewTrack->_SkeletonInfo = _SkeletonInfo;

	// NB: Output binding is not cloned for now. Need?

	return NewTrack;
}
//---------------------------------------------------------------------

// FIXME: this logic is the same for any clip/track type
float CPoseTrack::GetDuration() const
{
	return _Clips.empty() ? 0.f : _Clips.back().EndTime;
}
//---------------------------------------------------------------------

// FIXME: this logic is the same for any clip/track type!
//???are both time points inclusive? event or action exactly at point will be executed twice then, not good
void CPoseTrack::PlayInterval(float PrevTime, float CurrTime, bool IsLast)
{
	if (_Clips.empty() || !_Output) return;

	const float MinTime = std::min(PrevTime, CurrTime);
	const float MaxTime = std::max(PrevTime, CurrTime);

	// Find range of active clips [ Clip.EndTime >= MinTime; Clip.StartTime > MaxTime )
	auto It = std::upper_bound(_Clips.begin(), _Clips.end(), MinTime, [](float Time, const CClip& Clip) { return Time < Clip.EndTime; });
	if (It == _Clips.end()) return;
	auto ItEnd = std::lower_bound(It + 1, _Clips.end(), MaxTime, [](const CClip& Clip, float Time) { return Time < Clip.StartTime; });
	if (It == ItEnd) return;

	if (PrevTime < CurrTime)
	{
		// Forward playback
		do
		{
			auto CurrIt = It++;
			CurrIt->Clip->PlayInterval(PrevTime - CurrIt->StartTime, CurrTime - CurrIt->StartTime, IsLast && (It == ItEnd), *_Output);
		}
		while (It != ItEnd);
	}
	else
	{
		// Backward playback
		auto RItEnd = std::make_reverse_iterator(It);
		auto RIt = std::make_reverse_iterator(ItEnd);
		do
		{
			auto CurrIt = RIt++;
			CurrIt->Clip->PlayInterval(PrevTime - CurrIt->StartTime, CurrTime - CurrIt->StartTime, IsLast && (RIt == RItEnd), *_Output);
		}
		while (RIt != RItEnd);
	}
}
//---------------------------------------------------------------------

}
