#pragma once
#ifndef __DEM_L2_AI_MEMORY_SYSTEM_H__
#define __DEM_L2_AI_MEMORY_SYSTEM_H__

#include "MemFact.h"
#include <Data/LinkedListSet.h>
#include <AI/ActorFwd.h>

// Memory stores facts known by the actor. Memory also provides the interface to query
// for facts by different criteria.

namespace AI
{
typedef CLinkedListSet<const Core::CRTTI*, PMemFact> CMemFactListSet;
typedef CMemFactListSet::CElement CMemFactNode;

class CMemSystem //: public Core::CRefCounted
{
protected:

	//typedef nDictionary<const Core::CRTTI*, nArray<class CSensor*>> CGroupedSensors;

	CActor*			pActor;
	
	CMemFactListSet Facts;
	//CGroupedSensors ValidationSensors;

	//???declare event OnSensorAdded/OnSensorRemoved in brain?

public:

	CMemSystem(CActor* Actor): pActor(Actor) {}

	//void			Init();
	void			Update();

	//???return CMemFactNode?
	CMemFact*		AddFact(const Core::CRTTI& Type);
	CMemFact*		FindFact(const CMemFact& Pattern, CFlags FieldMask = CFlags(0xffffffff));
	CMemFactNode*	GetFactsByType(const Core::CRTTI& Type) { return Facts.GetHead(&Type); }

	//???GetTotalConfidence(const Core::CRTTI& Type)?
};

}

#endif