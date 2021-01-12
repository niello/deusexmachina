#include "ClipPlayerNode.h"

namespace DEM::Anim
{

void CClipPlayerNode::Init(/*some params?*/)
{
	_CurrClipTime = _StartTime;
}
//---------------------------------------------------------------------

void CClipPlayerNode::Update(float dt/*, params*/)
{
	_CurrClipTime += dt * _Speed;

	//!!!clamp / loop!
	//???where to handle synchronization? pass sync info in a context?
}
//---------------------------------------------------------------------

void CClipPlayerNode::EvaluatePose(IPoseOutput& Output)
{
	_Sampler.Apply(_CurrClipTime, Output);
}
//---------------------------------------------------------------------

}
