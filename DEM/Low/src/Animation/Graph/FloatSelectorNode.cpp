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
	_ParamHandle = Context.Controller.GetParams().Find<float>(_ParamID);

	//for (auto& Rec : _Variants)
	//	if (Rec.Variant.Node) Rec.Variant.Node->Init(Context);

	//if (_DefaultVariant.Node) _DefaultVariant.Node->Init(Context);

	CSelectorNodeBase::Init(Context);
}
//---------------------------------------------------------------------

CSelectorNodeBase::CVariant* CFloatSelectorNode::SelectVariant(CAnimationUpdateContext& Context)
{
	if (!_ParamHandle) return nullptr;
	return nullptr; // Context.Controller.GetBool(_ParamIndex) ? &_TrueVariant : &_FalseVariant;
}
//---------------------------------------------------------------------

}
