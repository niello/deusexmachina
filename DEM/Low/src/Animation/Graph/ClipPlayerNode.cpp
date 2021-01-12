#include "ClipPlayerNode.h"
#include <Animation/AnimationClip.h>

namespace DEM::Anim
{

void CClipPlayerNode::Init(/*some params? e.g. for clip asset obtaining*/)
{
	_CurrClipTime = _StartTime;

	//_Sampler.SetClip(Clip);
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
