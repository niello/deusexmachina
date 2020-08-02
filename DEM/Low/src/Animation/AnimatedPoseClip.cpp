#pragma once
#include "AnimatedPoseClip.h"
#include <Animation/AnimationClip.h>
#include <Animation/NodeMapping.h>
#include <Animation/MappedPoseOutput.h>

namespace DEM::Anim
{

void CAnimatedPoseClip::BindToOutput(const PPoseOutput& Output)
{
	if (!_Player || !_Player->GetClip() || _Output == Output) return;

	std::vector<U16> PortMapping;
	_Player->GetClip()->GetNodeMapping().Bind(*Output, PortMapping);
	if (PortMapping.empty())
		_Output = Output;
	else
		_Output = n_new(DEM::Anim::CMappedPoseOutput(PPoseOutput(Output), std::move(PortMapping)));
}
//---------------------------------------------------------------------

void CAnimatedPoseClip::PlayInterval(float PrevTime, float CurrTime, const CPoseTrack& Track, UPTR ClipIndex)
{
	//sample pose at the current time, can safely ignore previous time
}
//---------------------------------------------------------------------

}
