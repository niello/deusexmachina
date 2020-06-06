#pragma once
#include <Game/Interaction/Target.h>
#include <Data/StringID.h>

// Target for interaction with entities in a game level

namespace Game
{

class CTargetEntity: public ITarget
{
public:

	CStrID EntityID;
};

}
