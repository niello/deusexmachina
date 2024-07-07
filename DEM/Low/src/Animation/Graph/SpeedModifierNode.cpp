#include "SpeedModifierNode.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

CSpeedModifierNode::CSpeedModifierNode(PAnimGraphNode&& Subgraph, CStrID ParamID, float FallbackMultiplier)
	: _Subgraph(std::move(Subgraph))
	, _ParamID(ParamID)
	, _FallbackMultiplier(FallbackMultiplier)
{
}
//---------------------------------------------------------------------

CSpeedModifierNode::CSpeedModifierNode(PAnimGraphNode&& Subgraph, float Multiplier)
	: _Subgraph(std::move(Subgraph))
	, _FallbackMultiplier(Multiplier)
{
}
//---------------------------------------------------------------------

void CSpeedModifierNode::Init(CAnimationInitContext& Context)
{
	_ParamHandle = Context.Controller.GetParams().Find<float>(_ParamID);

	if (_Subgraph) _Subgraph->Init(Context);
}
//---------------------------------------------------------------------

void CSpeedModifierNode::Update(CAnimationUpdateContext& Context, float dt)
{
	if (_Subgraph) _Subgraph->Update(Context, dt * Context.Controller.GetParams().Get<float>(_ParamHandle, _FallbackMultiplier));
}
//---------------------------------------------------------------------

void CSpeedModifierNode::EvaluatePose(CPoseBuffer& Output)
{
	if (_Subgraph) _Subgraph->EvaluatePose(Output);
	//???otherwise reset to ref pose / additive zero?
}
//---------------------------------------------------------------------

}
