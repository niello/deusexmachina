#pragma once
#include <AI/Memory/MemFact.h>
#include <Data/StringID.h>

// Memory fact representing overseer, entity that controls this actor's behavoiur

namespace AI
{
typedef Ptr<class CStimulus> PStimulus;

class CMemFactOverseer: public CMemFact
{
	FACTORY_CLASS_DECL;

public:

	PStimulus		pSourceStimulus; //???to CMemFact?

	//???Position to validate this fact by vision?
	//???need validation or simple forgetting is enough?

	virtual bool Match(const CMemFact& Pattern, Data::CFlags FieldMask) const;
};

typedef Ptr<CMemFactOverseer> PMemFactOverseer;

}
