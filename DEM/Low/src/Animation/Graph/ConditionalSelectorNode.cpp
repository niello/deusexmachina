#include "ConditionalSelectorNode.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

CConditionalSelectorNode::CConditionalSelectorNode(CStrID ParamID)
	: _ParamID(ParamID)
{
}
//---------------------------------------------------------------------

void CConditionalSelectorNode::Init(CAnimationInitContext& Context)
{
	EParamType ParamType;
	if (!Context.Controller.FindParam(_ParamID, &ParamType, &_ParamIndex) || ParamType != EParamType::Bool)
		_ParamIndex = INVALID_INDEX;

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CConditionalSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (_ParamIndex == INVALID_INDEX) return nullptr;
	return Context.Controller.GetBool(_ParamIndex) ? &_TrueVariant : &_FalseVariant;
}
//---------------------------------------------------------------------

}
