#include "ClipPlayerNode.h"
#include <Animation/AnimationClip.h>
#include <Animation/SkeletonInfo.h>

namespace DEM::Anim
{

void CClipPlayerNode::Init(/*some params? e.g. for clip asset obtaining*/)
{
	_CurrClipTime = _StartTime;

	PAnimationClip Clip;
	// get clip, using remapper and resource manager
	if (Clip)
	{
		//std::vector<U16> PortMapping;
		//Clip->GetSkeletonInfo().MergeInto(SkeletonInfo, PortMapping);

		//!!!PortMapping must be used in EvaluatePose if not empty!
		//???use stack-based CMappedPoseOutput?
		//???!!!store fixed?! unique_ptr<[]>, nullptr = direct mapping

		_Sampler.SetClip(Clip);
	}
}
//---------------------------------------------------------------------

void CClipPlayerNode::Update(float dt/*, params*/)
{
	if (_Sampler.GetClip())
		_CurrClipTime = _Sampler.GetClip()->AdjustTime(_CurrClipTime + dt * _Speed, _Loop);

	//???where to handle synchronization? pass sync info into a context and sync after graph update?
}
//---------------------------------------------------------------------

void CClipPlayerNode::EvaluatePose(IPoseOutput& Output)
{
	_Sampler.Apply(_CurrClipTime, Output);
}
//---------------------------------------------------------------------

}
