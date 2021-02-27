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
	EParamType ParamType;
	if (!Context.Controller.FindParam(_ParamID, &ParamType, &_ParamIndex) || ParamType != EParamType::Bool)
		_ParamIndex = INVALID_INDEX;

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CStringSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (_ParamIndex == INVALID_INDEX) return nullptr;
	const CStrID Value = Context.Controller.GetString(_ParamIndex);

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
