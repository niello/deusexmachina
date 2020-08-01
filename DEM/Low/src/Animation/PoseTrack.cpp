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

	for (auto& Clip : _Clips)
		Clip->BindToOutput(_Output.get());
}
//---------------------------------------------------------------------

void CPoseTrack::PlayInterval(float PrevTime, float CurrTime)
{
	//!!!if interval is wrapped when looping, process both parts, but not separately, must not do 2 updates!
	//if end time > duration or < start time, is it a robust sign of looping?

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
