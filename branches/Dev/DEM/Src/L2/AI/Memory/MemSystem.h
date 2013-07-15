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
typedef CMemFactListSet::CIterator CMemFactNode;

class CMemSystem //: public Core::CRefCounted
{
protected:

	//typedef CDict<const Core::CRTTI*, CArray<class CSensor*>> CGroupedSensors;

	CActor*			pActor;
	
	CMemFactListSet Facts;
	//CGroupedSensors ValidationSensors;

	//???declare event OnSensorAdded/OnSensorRemoved in brain?

public:

	CMemSystem(CActor* Actor): pActor(Actor) {}

	//void				Init();
	void				Update();

	//???return CMemFactNode?
	template<class T>
	T*					AddFact();
	CMemFact*			FindFact(const CMemFact& Pattern, CFlags FieldMask = CFlags(0xffffffff));
	CMemFactNode		GetFactsByType(const Core::CRTTI& Type) { return Facts.GetHead(&Type); }

	//???GetTotalConfidence(const Core::CRTTI& Type)?
};

template<class T> inline T* CMemSystem::AddFact()
{
	// remember list count
	CMemFactNode Node = Facts.Add(n_new(T));
	// if list count changed, build validation sensor list for Type
	return (T*)Node->GetUnsafe();
}
//---------------------------------------------------------------------

}

#endif