#pragma once
#ifndef __IPG_AI_VALIDATOR_INVENTORY_UI_STATUS_H__
#define __IPG_AI_VALIDATOR_INVENTORY_UI_STATUS_H__

#include <AI/SmartObj/Validator.h>

// Checks is inventory window still open

namespace AI
{

class CValidatorContainerUIStatus: public CValidator
{
	DeclareRTTI;
	DeclareFactory(CValidatorContainerUIStatus);

public:

	virtual bool IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction);
};

RegisterFactory(CValidatorContainerUIStatus);

typedef Ptr<CValidatorContainerUIStatus> PValidatorContainerUIStatus;

}

#endif