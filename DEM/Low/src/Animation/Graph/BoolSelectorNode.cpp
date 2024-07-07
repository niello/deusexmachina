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
	_ParamHandle = Context.Controller.GetParams().Find<bool>(_ParamID);

	if (_TrueVariant.Node) _TrueVariant.Node->Init(Context);
	if (_FalseVariant.Node) _FalseVariant.Node->Init(Context);

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CBoolSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (!_ParamHandle) return nullptr;
	return Context.Controller.GetParams().Get<bool>(_ParamHandle) ? &_TrueVariant : &_FalseVariant;
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
