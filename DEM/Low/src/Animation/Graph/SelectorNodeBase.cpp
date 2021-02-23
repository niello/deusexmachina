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
	auto pNewVariant = SelectVariant(); //???interruption must be checked inside? if _pCurrVariant is not null
	if (pNewVariant != _pCurrVariant)
	{
		if (pNewVariant) Context.Controller.RequestInertialization(pNewVariant->BlendTime);
		_pCurrVariant = pNewVariant;
	}

	/*
	for (UPTR i = 0; i < _Variants.size(); ++i)
	{
		if (i == _CurrVariantIndex) continue;
		// if (_CurrVariant != INVALID_INDEX && CantInterruptIt) continue;
		// if (Condition) return variant;
	}
	*/

	//!!!if no condition was triggered, check if the current variant condition is still true?
	//???or check if the current variant is finished and reset _CurrVariant to an empty value if so.

	if (_pCurrVariant)
		if (auto pNode = _pCurrVariant->Node.get())
			pNode->Update(Context, dt);
}
//---------------------------------------------------------------------

void CSelectorNodeBase::EvaluatePose(CPoseBuffer& Output)
{
	if (_pCurrVariant)
		if (auto pNode = _pCurrVariant->Node.get())
			pNode->EvaluatePose(Output);

	//???otherwise reset to ref pose / additive zero?
}
//---------------------------------------------------------------------

float CSelectorNodeBase::GetAnimationLengthScaled() const
{
	if (!_pCurrVariant) return 0.f;
	const auto pNode = _pCurrVariant->Node.get();
	return pNode ? pNode->GetAnimationLengthScaled() : 0.f;
}
//---------------------------------------------------------------------

}
