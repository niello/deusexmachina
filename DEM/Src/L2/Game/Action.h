#pragma once
#ifndef __DEM_L2_GAME_ACTION_H__
#define __DEM_L2_GAME_ACTION_H__

#include <StdDEM.h>

// Action encapsulates interactional game logic. When player uses
// some ability on the target, action describes what exactly happens.

namespace Game
{
struct CActionContext;
class ITarget;

class IAction
{
protected:

public:

	virtual bool	IsAvailable(const CActionContext& Context, const ITarget& Target) const = 0;
	virtual bool	Execute(const CActionContext& Context, const ITarget& Target) const = 0;
};

}

#endif
