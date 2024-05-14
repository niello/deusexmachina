#include "SelectorNodeBase.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

void CSelectorNodeBase::Init(CAnimationInitContext& Context)
{
	_pCurrVariant = nullptr;
}
//---------------------------------------------------------------------

void CSelectorNodeBase::Update(CAnimationUpdateContext& Context, float dt)
{
	// FIXME: need to remember why was this added. This breaks inertialization blending from finished one-off animations to new selection.
	//!!!probably this is needed in case that this node was not active at all for some time, then we need not to blend from _pCurrVariant
	// because we aren't really in it anymore!
	// If current subnode finished its evaluation, reset state and select the new one without limitations
	//if (_pCurrVariant && !_pCurrVariant->Node->IsActive())
	//	_pCurrVariant = nullptr;

	auto pNewVariant = SelectVariant(Context);
	if (pNewVariant && !pNewVariant->Node) pNewVariant = nullptr;
	if (pNewVariant != _pCurrVariant)
	{
		// NB: <= to allow children with the same priority to interrupt each other
		if (!pNewVariant || !_pCurrVariant || _pCurrVariant->InterruptionPriority <= pNewVariant->InterruptionPriority)
		{
			if (_pCurrVariant && pNewVariant) Context.Controller.RequestInertialization(pNewVariant->BlendTime);
			_pCurrVariant = pNewVariant;
		}
	}

	if (_pCurrVariant) _pCurrVariant->Node->Update(Context, dt);
}
//---------------------------------------------------------------------

void CSelectorNodeBase::EvaluatePose(CPoseBuffer& Output)
{
	if (_pCurrVariant) _pCurrVariant->Node->EvaluatePose(Output);
	//???otherwise reset to ref pose / additive zero?
}
//---------------------------------------------------------------------

}
