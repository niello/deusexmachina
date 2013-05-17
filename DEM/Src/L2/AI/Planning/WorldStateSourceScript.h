#pragma once
#ifndef __DEM_L2_AI_WORLD_STATE_SOURCE_SCRIPT_H__
#define __DEM_L2_AI_WORLD_STATE_SOURCE_SCRIPT_H__

#include "WorldStateSource.h"
#include <Data/SimpleString.h>

// Fills world state from the conventional script output.

namespace AI
{

class CWorldStateSourceScript: public CWorldStateSource
{
	__DeclareClass(CWorldStateSourceScript);

protected:

	Data::CSimpleString Func;

public:

	virtual void Init(Data::PParams Desc);
	virtual bool FillWorldState(const CActor* pActor, const CPropSmartObject* pSO, CWorldState& WS);
};

typedef Ptr<CWorldStateSourceScript> PWorldStateSourceScript;

}

#endif