#include "AnimatedPoseClip.h"
#include <Animation/AnimationClip.h>
#include <Animation/NodeMapping.h>
#include <Animation/MappedPoseOutput.h>

namespace DEM::Anim
{

void CAnimatedPoseClip::SetAnimationClip(const PAnimationClip& Clip)
{
	_Sampler.SetClip(Clip);
}
//---------------------------------------------------------------------

void CAnimatedPoseClip::BindToOutput(const PPoseOutput& Output)
{
	if (!_Sampler.GetClip() || _Output == Output) return;

	std::vector<U16> PortMapping;
	_Sampler.GetClip()->GetNodeMapping().Bind(*Output, PortMapping);
	if (PortMapping.empty())
		_Output = Output;
	else
		_Output = n_new(DEM::Anim::CMappedPoseOutput(PPoseOutput(Output), std::move(PortMapping)));
}
//---------------------------------------------------------------------

void CAnimatedPoseClip::PlayInterval(float /*PrevTime*/, float CurrTime, bool IsLast, const CPoseTrack& /*Track*/, UPTR /*ClipIndex*/)
{
	//!!!FIXME: sample pose only if CurrTime is inside the clip (>=0 && <= duration)! Otherwise clip is skipped already.
	//???IsLast will already check it? Last clip is always current, otherwise it is skipped. Rename to IsCurr?
	if (IsLast && _Output) _Sampler.Apply(CurrTime, *_Output);
}
//---------------------------------------------------------------------

}