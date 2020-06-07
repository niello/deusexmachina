#pragma once
#include <Game/Interaction/Target.h>
#include <Data/StringID.h>

// Target for interaction with entities in a game level

namespace DEM::Game
{

class CTargetEntity: public CTarget
{
public:

	CStrID EntityID;
};

}
