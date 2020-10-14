#pragma once
#ifndef __DEM_L2_AI_WORLD_STATE_SOURCE_H__
#define __DEM_L2_AI_WORLD_STATE_SOURCE_H__

#include <Core/Object.h>
#include <AI/ActorFwd.h>
#include <AI/Planning/WorldState.h>
#include <Data/Params.h>

// World state source dynamically provides new or patches existing CWorldState.
// E.g. it is very useful for getting SO action preconditions and effects from script.

//???AI/SmartObjFwd.h?
namespace Prop
{
	class CPropSmartObject;
}

namespace AI
{

class CWorldState;

class CWorldStateSource: public Core::CObject
{
	RTTI_CLASS_DECL(AI::CWorldStateSource, Core::CObject);

public:

	virtual void Init(Data::PParams Desc) = 0;
	virtual bool FillWorldState(const CActor* pActor, const Prop::CPropSmartObject* pSO, CWorldState& WS) = 0;
};

typedef Ptr<CWorldStateSource> PWorldStateSource;

}

#endif
