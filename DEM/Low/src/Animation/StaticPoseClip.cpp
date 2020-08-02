#pragma once
#include "StaticPoseClip.h"
#include <Animation/NodeMapping.h>
#include <Animation/MappedPoseOutput.h>

namespace DEM::Anim
{

void CStaticPoseClip::BindToOutput(const PPoseOutput& Output)
{
	if (!_Pose || _Output == Output) return;

	std::vector<U16> PortMapping;
	_Pose->GetNodeMapping().Bind(*Output, PortMapping);
	if (PortMapping.empty())
		_Output = Output;
	else
		_Output = n_new(DEM::Anim::CMappedPoseOutput(PPoseOutput(Output), std::move(PortMapping)));
}
//---------------------------------------------------------------------

void CStaticPoseClip::PlayInterval(float /*PrevTime*/, float /*CurrTime*/, const CPoseTrack& /*Track*/, UPTR /*ClipIndex*/)
{
	if (_Output && _Pose) _Pose->Apply(*_Output);
}
//---------------------------------------------------------------------

}
