#include "PoseTrack.h"
#include <Animation/PoseClipBase.h>
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{
CPoseTrack::CPoseTrack() = default;
CPoseTrack::~CPoseTrack() = default;

//!!!recurse through blenders, handle mapped outputs for leaf sources (like in loading now).
void CPoseTrack::SetOutput(const PPoseOutput& Output)
{
	if (Output == _Output) return;

	_Output = Output;

	for (auto& [Clip, StartTime, EndTime] : _Clips)
		Clip->BindToOutput(_Output);
}
//---------------------------------------------------------------------

// FIXME: this logic is the same for any clip/track type
void CPoseTrack::AddClip(PPoseClipBase&& Clip, float StartTime, float Duration /*, overlap resolve mode*/)
{
	if (!Clip || Duration <= 0.f) return;
	auto It = std::lower_bound(_Clips.begin(), _Clips.end(), StartTime, [](const CClip& Clip, float Time) { return Clip.EndTime < Time; });
	_Clips.insert(It, CClip{ std::move(Clip), StartTime, StartTime + Duration });
}
//---------------------------------------------------------------------

PTimelineTrack CPoseTrack::Clone() const
{
	NOT_IMPLEMENTED;
	return nullptr;
}
//---------------------------------------------------------------------

// FIXME: this logic is the same for any clip/track type
float CPoseTrack::GetDuration() const
{
	return _Clips.empty() ? 0.f : _Clips.back().EndTime;
}
//---------------------------------------------------------------------

// FIXME: this logic is the same for any clip/track type
//???are both time points inclusive? event or action exactly at point will be executed twice then, not good
void CPoseTrack::PlayInterval(float PrevTime, float CurrTime, bool IsLast)
{
	if (_Clips.empty()) return;

	if (PrevTime < CurrTime)
	{
		// Forward playback
		auto It = std::upper_bound(_Clips.begin(), _Clips.end(), PrevTime, [](float Time, const CClip& Clip) { return Time < Clip.EndTime; });
		bool IsLastClip = (It == _Clips.end());
		while (!IsLastClip)
		{
			auto CurrIt = It++;
			IsLastClip = (It == _Clips.end() || It->StartTime >= CurrTime);
			const float Offset = CurrIt->StartTime;
			CurrIt->Clip->PlayInterval(PrevTime - Offset, CurrTime - Offset,
				IsLast && IsLastClip, *this, std::distance(_Clips.begin(), CurrIt));
		}
	}
	else
	{
		// Backward playback
		NOT_IMPLEMENTED;
	}
}
//---------------------------------------------------------------------

}
