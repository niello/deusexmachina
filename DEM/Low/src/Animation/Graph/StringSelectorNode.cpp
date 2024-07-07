#include "StringSelectorNode.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

CStringSelectorNode::CStringSelectorNode(CStrID ParamID)
	: _ParamID(ParamID)
{
}
//---------------------------------------------------------------------

void CStringSelectorNode::Init(CAnimationInitContext& Context)
{
	_ParamHandle = Context.Controller.GetParams().Find<CStrID>(_ParamID);

	for (auto& Rec : _Variants)
		if (Rec.Variant.Node) Rec.Variant.Node->Init(Context);

	if (_DefaultVariant.Node) _DefaultVariant.Node->Init(Context);

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CStringSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (!_ParamHandle) return nullptr;
	const CStrID Value = Context.Controller.GetParams().Get<CStrID>(_ParamHandle);

	// TODO: sort variants?
	for (auto& Rec : _Variants)
		if (Value == Rec.Value)
			return &Rec.Variant;

	return &_DefaultVariant;
}
//---------------------------------------------------------------------

void CStringSelectorNode::AddVariant(PAnimGraphNode&& Node, CStrID Value, float BlendTime, U32 InterruptionPriority)
{
	// TODO: sort variants?
	for (auto& Rec : _Variants)
	{
		if (Value == Rec.Value)
		{
			Rec.Variant.Node = std::move(Node);
			Rec.Variant.BlendTime = BlendTime;
			Rec.Variant.InterruptionPriority = InterruptionPriority;
			return;
		}
	}

	_Variants.push_back({ { std::move(Node), BlendTime, InterruptionPriority }, Value });
}
//---------------------------------------------------------------------

void CStringSelectorNode::SetDefaultVariant(PAnimGraphNode&& Node, float BlendTime, U32 InterruptionPriority)
{
	_DefaultVariant.Node = std::move(Node);
	_DefaultVariant.BlendTime = BlendTime;
	_DefaultVariant.InterruptionPriority = InterruptionPriority;
}
//---------------------------------------------------------------------

}
