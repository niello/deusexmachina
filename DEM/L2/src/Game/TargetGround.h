#pragma once
#ifndef __DEM_L2_GAME_TARGET_GROUND_H__
#define __DEM_L2_GAME_TARGET_GROUND_H__

#include <Game/Target.h>
#include <Math/vector3.h>

// Target for interaction with a static level geometry

namespace Game
{

class CTargetGround: public ITarget
{
protected:

	UPTR	TypeFlag;
	vector3 Position;
	//???other info about the ground at the point of interaction?

public:

	CTargetGround(const vector3& TargetPosition): Position(TargetPosition) { TypeFlag = TargetTypes.GetMask(GetTypeID().CStr()); }

	virtual CStrID	GetTypeID() const;
	virtual UPTR	GetAvailableActions(const CActionContext& Context, CArray<IAction*>& OutActions) const;
	virtual void	GetParams(Data::CParams& Out) const;
};

}

#endif
