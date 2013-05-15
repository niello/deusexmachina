#pragma once
#ifndef __DEM_L2_AI_MEM_FACT_H__
#define __DEM_L2_AI_MEM_FACT_H__

#include <StdDEM.h>
#include <Core/RefCounted.h>
#include <Data/Flags.h>

// Memory fact is a knowledge symbol instance, used for decision making. Symbols represent
// the world from the actor's point of view. Examples of symbol: Enemy, Item, Danger etc.
// Derive from this class to create specific symbols understandable by the agent.

namespace AI
{
using namespace Data;

class CMemFact: public Core::CRefCounted
{
	__DeclareClassNoFactory;

protected:

	//???UID to Match directly?

public:

	float Confidence;
	float LastPerceptionTime;
	float LastUpdateTime;
	float ForgettingFactor;	// Decrease of confidence per second
	
	virtual bool	Match(const CMemFact& Pattern, CFlags FieldMask) const;
	
	// For CLinkedListSet
	Core::CRTTI*	GetKey() const { return GetRTTI(); }

	bool operator <(const CMemFact& Other) const { return Confidence < Other.Confidence; }
	bool operator >(const CMemFact& Other) const { return Confidence > Other.Confidence; }
	bool operator <=(const CMemFact& Other) const { return Confidence <= Other.Confidence; }
	bool operator >=(const CMemFact& Other) const { return Confidence >= Other.Confidence; }
};

typedef Ptr<CMemFact> PMemFact;

}

#endif