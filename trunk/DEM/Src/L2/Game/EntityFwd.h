#pragma once
#ifndef __DEM_L2_GAME_ENTITY_FWD_H__
#define __DEM_L2_GAME_ENTITY_FWD_H__

#include <Data/StringID.h>

// Entity support - lightweight forward declarations & small helper classes

namespace Game
{

enum EntityPool
{
	LivePool = 1,
	SleepingPool = 2
};

class CEntity;

}

#endif
