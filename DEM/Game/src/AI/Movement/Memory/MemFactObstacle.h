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

public:

	PStimulus		pSourceStimulus; //???to CMemFact?
	vector3			Position;
	float			Radius = 0.f;
	//???float Height;

	CMemFactObstacle();
	virtual ~CMemFactObstacle() override;

	virtual bool Match(const CMemFact& Pattern, Data::CFlags FieldMask) const;
};

typedef Ptr<CMemFactObstacle> PMemFactObstacle;

}
