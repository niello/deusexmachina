#include "IntSelectorNode.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

CIntSelectorNode::CIntSelectorNode(CStrID ParamID)
	: _ParamID(ParamID)
{
}
//---------------------------------------------------------------------

void CIntSelectorNode::Init(CAnimationInitContext& Context)
{
	_ParamHandle = Context.Controller.GetParams().Find<int>(_ParamID);

	for (auto& Rec : _Variants)
		if (Rec.Variant.Node) Rec.Variant.Node->Init(Context);

	//if (_DefaultVariant.Node) _DefaultVariant.Node->Init(Context);

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CIntSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (!_ParamHandle) return nullptr;
	const int Value = Context.Controller.GetParams().Get<int>(_ParamHandle);

	// TODO: sort variants?
	for (auto& Rec : _Variants)
		if (Rec.From <= Value && Value <= Rec.To)
			return &Rec.Variant;

	return nullptr;
}
//---------------------------------------------------------------------

void CIntSelectorNode::AddVariant(PAnimGraphNode&& Node, int From, int To, float BlendTime, U32 InterruptionPriority)
{
	// TODO: sort variants? DEFAULT variant must be From INT_MIN To INT_MAX and must be the last.
	// E.g. can sort by To ascending.
	_Variants.push_back({ { std::move(Node), BlendTime, InterruptionPriority }, From, To });
}
//---------------------------------------------------------------------

}
