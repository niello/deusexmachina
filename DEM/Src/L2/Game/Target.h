#pragma once
#ifndef __DEM_L2_GAME_TARGET_H__
#define __DEM_L2_GAME_TARGET_H__

#include <Data/Ptr.h>
#include <Data/DynamicEnum.h>

// Target can accept action context to determine available actions.
// It is an abstraction above all possible action targets through
// any game mechanics. The most typical targets are level geometry
// and entities. Some games may target player avatar as separate type.

namespace Data
{
	class CParams;
}

namespace Game
{
struct CActionContext;
class IAction;

class ITarget
{
public:

	static Data::CDynamicEnum TargetTypes;

	virtual CStrID	GetTypeID() const = 0;
	virtual UPTR	GetAvailableActions(const CActionContext& Context, CArray<IAction*>& OutActions) const = 0;
	virtual void	GetParams(Data::CParams& Out) const = 0;
};

}

#endif
