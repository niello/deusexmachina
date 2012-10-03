#pragma once
#ifndef __DEM_L2_AI_MEM_FACT_OBSTACLE_H__
#define __DEM_L2_AI_MEM_FACT_OBSTACLE_H__

#include <AI/Memory/MemFact.h>
#include <Data/StringID.h>
#include <mathlib/vector.h>

// Memory fact representing obstacle

namespace AI
{
typedef Ptr<class CStimulus> PStimulus;

class CMemFactObstacle: public CMemFact
{
	DeclareRTTI;
	DeclareFactory(CMemFactObstacle);

protected:

public:

	PStimulus		pSourceStimulus; //???to CMemFact?
	vector3			Position;
	float			Radius;
	//???float Height;

	CMemFactObstacle(): Radius(0.f) { /*Type = CStrID("Obstacle");*/ }

	virtual bool Match(const CMemFact& Pattern, CFlags FieldMask) const;
};

RegisterFactory(CMemFactObstacle);

typedef Ptr<CMemFactObstacle> PMemFactObstacle;

}

#endif