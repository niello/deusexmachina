#include "BoolSelectorNode.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

CBoolSelectorNode::CBoolSelectorNode(CStrID ParamID)
	: _ParamID(ParamID)
{
}
//---------------------------------------------------------------------

void CBoolSelectorNode::Init(CAnimationInitContext& Context)
{
	EParamType ParamType;
	if (!Context.Controller.FindParam(_ParamID, &ParamType, &_ParamIndex) || ParamType != EParamType::Bool)
		_ParamIndex = INVALID_INDEX;

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CBoolSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (_ParamIndex == INVALID_INDEX) return nullptr;
	return Context.Controller.GetBool(_ParamIndex) ? &_TrueVariant : &_FalseVariant;
}
//---------------------------------------------------------------------

void CBoolSelectorNode::SetTrueNode(PAnimGraphNode&& Node, float BlendTime, U32 InterruptionPriority)
{
	_TrueVariant.Node = std::move(Node);
	_TrueVariant.BlendTime = BlendTime;
	_TrueVariant.InterruptionPriority = InterruptionPriority;
}
//---------------------------------------------------------------------

void CBoolSelectorNode::SetFalseNode(PAnimGraphNode&& Node, float BlendTime, U32 InterruptionPriority)
{
	_FalseVariant.Node = std::move(Node);
	_FalseVariant.BlendTime = BlendTime;
	_FalseVariant.InterruptionPriority = InterruptionPriority;
}
//---------------------------------------------------------------------

}
