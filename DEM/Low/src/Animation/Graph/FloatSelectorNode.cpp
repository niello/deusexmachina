#include "FloatSelectorNode.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

CFloatSelectorNode::CFloatSelectorNode(CStrID ParamID)
	: _ParamID(ParamID)
{
}
//---------------------------------------------------------------------

void CFloatSelectorNode::Init(CAnimationInitContext& Context)
{
	EParamType ParamType;
	if (!Context.Controller.FindParam(_ParamID, &ParamType, &_ParamIndex) || ParamType != EParamType::Bool)
		_ParamIndex = INVALID_INDEX;

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CFloatSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (_ParamIndex == INVALID_INDEX) return nullptr;
	return nullptr; // Context.Controller.GetBool(_ParamIndex) ? &_TrueVariant : &_FalseVariant;
}
//---------------------------------------------------------------------

}
