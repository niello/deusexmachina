#pragma once
#ifndef __IPG_AI_VALIDATOR_PLR_ONLY_H__
#define __IPG_AI_VALIDATOR_PLR_ONLY_H__

#include <AI/SmartObj/Validator.h>

// Checks is an actor a player-controlled one

namespace AI
{

class CValidatorPlrOnly: public CValidator
{
	__DeclareClass(CValidatorPlrOnly);

public:

	virtual bool IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction);
};

typedef Ptr<CValidatorPlrOnly> PValidatorPlrOnly;

}

#endif