#include "PoseTrack.h"
#include <Animation/PoseClipBase.h>
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{
CPoseTrack::CPoseTrack() = default;
CPoseTrack::~CPoseTrack() = default;

//!!!recurse through blenders, handle mapped outputs for leaf sources (like in loading now).
void CPoseTrack::SetOutput(PPoseOutput&& Output)
{
	if (Output == _Output) return;

	_Output = std::move(Output);

	for (auto& [Clip, StartTime, EndTime] : _Clips)
		Clip->BindToOutput(_Output);
}
//---------------------------------------------------------------------

float CPoseTrack::GetDuration() const
{
	return _Clips.empty() ? 0.f : _Clips.back().StartTime + _Clips.back().Clip->GetDuration();
}
//---------------------------------------------------------------------

// FIXME: this logic is the same for any clip/track type
void CPoseTrack::PlayInterval(float PrevTime, float CurrTime, bool IsLast)
{
	if (_Clips.empty()) return;

	auto It = std::upper_bound(_Clips.begin(), _Clips.end(), PrevTime, [](float Time, const CClip& Clip) { return Time < Clip.EndTime; });
	if (It != _Clips.end())
	{
		//...
	}

	// CurrTime < PrevTime if playing backwards
	// maybe multiple calls in the same frame
	// no wrapping, monotone linear space
	// NB: are both points inclusive? event or action exactly at point will be executed twice then, not good
	// find clip at prev time, don't play if prev time == clip start/end and a clip is outside the time range
	// go from this clip to the first clip ending (starting) outside the range

	// lower bound - first not less
	// upper bound - first greater
	// get clip indices for easier comparison and iteration?

	//!!!clips must be sorted by start time!
	for (size_t i = 0; i < _Clips.size(); ++i)
	{
		// if clip is in interval:
		// convert time to clip-local
		//Clip->PlayInterval(PrevTimeLocal, CurrTimeLocal, *this, i);
	}
}
//---------------------------------------------------------------------

}
