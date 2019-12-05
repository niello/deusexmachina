#pragma once
#include <AI/Memory/MemFact.h>
#include <Data/StringID.h>
#include <Math/Vector3.h>

// Memory fact representing obstacle

namespace AI
{
typedef Ptr<class CStimulus> PStimulus;

class CMemFactObstacle: public CMemFact
{
	FACTORY_CLASS_DECL;

protected:

public:

	PStimulus		pSourceStimulus; //???to CMemFact?
	vector3			Position;
	float			Radius;
	//???float Height;

	CMemFactObstacle(): Radius(0.f) { /*Type = CStrID("Obstacle");*/ }

	virtual bool Match(const CMemFact& Pattern, Data::CFlags FieldMask) const;
};

typedef Ptr<CMemFactObstacle> PMemFactObstacle;

}
