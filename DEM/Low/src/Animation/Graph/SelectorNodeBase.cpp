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
	// If current subnode finished its evaluation, reset state and select the new one without limitations
	if (_pCurrVariant && !_pCurrVariant->Node->IsActive())
		_pCurrVariant = nullptr;

	auto pNewVariant = SelectVariant(Context);
	if (pNewVariant && !pNewVariant->Node) pNewVariant = nullptr;
	if (pNewVariant != _pCurrVariant)
	{
		// NB: <= to allow children with the same priority to interrupt each other
		if (!pNewVariant || !_pCurrVariant || _pCurrVariant->InterruptionPriority <= pNewVariant->InterruptionPriority)
		{
			if (pNewVariant) Context.Controller.RequestInertialization(pNewVariant->BlendTime);
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
