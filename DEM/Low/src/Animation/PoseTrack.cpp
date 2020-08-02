#pragma once
#include "PoseTrack.h"
#include <Animation/PoseClipBase.h>
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{
CPoseTrack::CPoseTrack() = default;
CPoseTrack::~CPoseTrack() = default;

void CPoseTrack::SetOutput(PPoseOutput&& Output)
{
	if (Output == _Output) return;

	_Output = std::move(Output);

	for (auto& [Clip, StartTime] : _Clips)
		Clip->BindToOutput(_Output);
}
//---------------------------------------------------------------------

//!!!all time conversions here are the same for every clip/track type!
void CPoseTrack::PlayInterval(float PrevTime, float CurrTime)
{
	if (_Clips.empty()) return;

	//???convert time to track duration? loop/clamp.

	//!!!if interval is wrapped when looping, process both parts, but not separately, must not do 2 updates!
	//if end time > duration or < start time, is it a robust sign of looping?

	// search for first and last affected clips, using binary search?
/*
	auto It = std::lower_bound(Children.begin(), Children.end(), ChildName, [](const PSceneNode& Child, CStrID Name) { return Child->GetName() < Name; });
	if (It != Children.end() && (*It)->GetName() == ChildName)
	{
		if (Replace) It = Children.erase(It);
		else return *It;
	}

	PSceneNode Node = n_new(CSceneNode)(ChildName);
*/

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
