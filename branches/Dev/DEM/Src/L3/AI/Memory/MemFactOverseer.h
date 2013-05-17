#pragma once
#ifndef __DEM_L3_AI_MEM_FACT_OVERSEER_H__
#define __DEM_L3_AI_MEM_FACT_OVERSEER_H__

#include <AI/Memory/MemFact.h>
#include <Data/StringID.h>
#include <mathlib/vector.h>

// Memory fact representing overseer, entity that controls this actor's behavoiur

namespace AI
{
typedef Ptr<class CStimulus> PStimulus;

class CMemFactOverseer: public CMemFact
{
	__DeclareClass(CMemFactOverseer);

public:

	PStimulus		pSourceStimulus; //???to CMemFact?

	//???Position to validate this fact by vision?
	//???need validation or simple forgetting is enough?

	virtual bool Match(const CMemFact& Pattern, CFlags FieldMask) const;
};

typedef Ptr<CMemFactOverseer> PMemFactOverseer;

}

#endif