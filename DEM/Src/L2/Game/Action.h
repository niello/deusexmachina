#pragma once
#ifndef __DEM_L2_GAME_ACTION_H__
#define __DEM_L2_GAME_ACTION_H__

#include <StdDEM.h>

// Action encapsulates interactional game logic. When player uses
// some ability on the target, action describes what exactly happens.
// Action is stateless, all the state is stored in a context.
// Actions are used only for iteractions involving the game world.

namespace Game
{
struct CActionContext;

class IAction
{
protected:

public:

	virtual bool	IsAvailable(const CActionContext& Context) const = 0;
	virtual bool	Execute(const CActionContext& Context) const = 0;
};

}

#endif
