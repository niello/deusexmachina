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
	EParamType ParamType;
	if (!Context.Controller.FindParam(_ParamID, &ParamType, &_ParamIndex) || ParamType != EParamType::Float)
		_ParamIndex = INVALID_INDEX;

	if (_Subgraph) _Subgraph->Init(Context);
}
//---------------------------------------------------------------------

void CSpeedModifierNode::Update(CAnimationUpdateContext& Context, float dt)
{
	//!!!DBG TMP!
	::Sys::DbgOut("***DBG dt = %lf, ActionSpeedMul = %lf\n", dt, Context.Controller.GetFloat(_ParamIndex, _FallbackMultiplier));

	if (_Subgraph) _Subgraph->Update(Context, dt * Context.Controller.GetFloat(_ParamIndex, _FallbackMultiplier));
}
//---------------------------------------------------------------------

void CSpeedModifierNode::EvaluatePose(CPoseBuffer& Output)
{
	if (_Subgraph) _Subgraph->EvaluatePose(Output);
	//???otherwise reset to ref pose / additive zero?
}
//---------------------------------------------------------------------

}
