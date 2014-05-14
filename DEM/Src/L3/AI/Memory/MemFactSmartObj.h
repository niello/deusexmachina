#pragma once
#ifndef __DEM_L3_AI_MEM_FACT_SMART_OBJ_H__
#define __DEM_L3_AI_MEM_FACT_SMART_OBJ_H__

#include <AI/Memory/MemFact.h>
#include <Data/StringID.h>

// Memory fact representing overseer, entity that controls this actor's behavoiur

namespace AI
{
typedef Ptr<class CStimulus> PStimulus;

class CMemFactSmartObj: public CMemFact
{
	__DeclareClass(CMemFactSmartObj);

public:

	PStimulus		pSourceStimulus; //???to CMemFact?
	CStrID			TypeID;

	//???Position to validate this fact by vision?
	//???need validation or simple forgetting is enough?

	virtual bool Match(const CMemFact& Pattern, Data::CFlags FieldMask) const;
};

typedef Ptr<CMemFactSmartObj> PMemFactSmartObj;

}

#endif