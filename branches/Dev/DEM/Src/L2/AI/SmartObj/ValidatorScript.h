#pragma once
#ifndef __DEM_L2_AI_VALIDATOR_SCRIPT_H__
#define __DEM_L2_AI_VALIDATOR_SCRIPT_H__

#include "Validator.h"
#include <Data/SimpleString.h>

// This validator runs scripted function of the smart object to obtain result

namespace AI
{

class CValidatorScript: public CValidator
{
	__DeclareClass(CValidatorScript);

protected:

	Data::CSimpleString ConditionFunc;
	Data::CSimpleString RelevanceFunc;

public:

	virtual void	Init(Data::PParams Desc);
	virtual bool	IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction);
	virtual float	GetRelevance(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction);
};

typedef Ptr<CValidatorScript> PValidatorScript;

}

#endif