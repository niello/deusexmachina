#pragma once
#ifndef __DEM_L2_GAME_TARGET_ENTITY_H__
#define __DEM_L2_GAME_TARGET_ENTITY_H__

#include <Game/Interaction/Target.h>

// Target for interaction with entities in a game level

namespace Game
{

class CTargetEntity: public ITarget
{
protected:

	UPTR	TypeFlag;
	CStrID	EntityID;

public:

	CTargetEntity(CStrID TargetID): EntityID(TargetID) { TypeFlag = TargetTypes.GetMask(GetTypeID().CStr()); }

	virtual CStrID	GetTypeID() const;
	virtual UPTR	GetAvailableActions(const CActionContext& Context, CArray<IAction*>& OutActions) const;
	virtual void	GetParams(Data::CParams& Out) const;
};

}

#endif
