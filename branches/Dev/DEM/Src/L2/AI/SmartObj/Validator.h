#pragma once
#ifndef __DEM_L2_AI_VALIDATOR_H__
#define __DEM_L2_AI_VALIDATOR_H__

#include <Core/RefCounted.h>
#include <AI/ActorFwd.h>
#include <Data/Params.h>

// Validator checks is action valid for a specific actor and also calculates relevance for task.
// Now this is designed for smart object actions, but may be will be used wider.

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

//???AI/SmartObjFwd.h?
namespace Properties
{
	class CPropSmartObject;
}

namespace AI
{
class CSmartObjAction;
using namespace Properties;

class CValidator: public Core::CRefCounted
{
	DeclareRTTI;

public:

	//???world state here? or relevance not here? relevance is planning!
	//can also get priority for UI action, as relevance or not

	virtual void	Init(Data::PParams Desc) {}
	virtual bool	IsValid(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction) = 0;
	virtual float	GetRelevance(const CActor* pActor, const CPropSmartObject* pSO, const CSmartObjAction* pAction) { return 0.f; }
};

typedef Ptr<CValidator> PValidator;

}

#endif