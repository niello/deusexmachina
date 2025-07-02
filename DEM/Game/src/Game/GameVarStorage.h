#pragma once
#include <Data/VarStorage.h>
#include <Game/ECS/Entity.h>
#include <rtm/vector4f.h> // to ensure that the type is complete

// A storage specialized for containing types useful for gameplay logic in addition to basic types

namespace DEM::Game
{
	using CGameVarStorage = CVarStorage<bool, int, float, std::string, CStrID, HEntity, rtm::vector4f>;
}
