#pragma once
#ifndef __DEM_L3_AI_VALIDATOR_DLG_RUNS_H__
#define __DEM_L3_AI_VALIDATOR_DLG_RUNS_H__

#include <AI/SmartObj/Validator.h>

// Checks if dialogue is running

namespace AI
{

class CValidatorDlgRuns: public CValidator
{
	__DeclareClass(CValidatorDlgRuns);

public:

	virtual bool IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction);
};

__RegisterClassInFactory(CValidatorDlgRuns);

typedef Ptr<CValidatorDlgRuns> PValidatorDlgRuns;

}

#endif