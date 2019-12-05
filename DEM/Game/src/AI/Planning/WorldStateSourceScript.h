#pragma once
#include "WorldStateSource.h"

// Fills world state from the conventional script output.

namespace AI
{

class CWorldStateSourceScript: public CWorldStateSource
{
	FACTORY_CLASS_DECL;

protected:

	CString Func;

public:

	virtual void Init(Data::PParams Desc);
	virtual bool FillWorldState(const CActor* pActor, const Prop::CPropSmartObject* pSO, CWorldState& WS);
};

typedef Ptr<CWorldStateSourceScript> PWorldStateSourceScript;

}
