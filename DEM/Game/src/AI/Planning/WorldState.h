#pragma once
#ifndef __DEM_L2_AI_WORLD_STATE_H__
#define __DEM_L2_AI_WORLD_STATE_H__

#include <Data/Data.h>

// World state is a metric for action planning algorithm. It symbolically describes most important
// parameters of the world, either in current, desired or some intermediate state.

namespace AI
{

enum EWSProp
{
	WSP_Invalid = -1,
	WSP_AtEntityPos = 0,
	WSP_UsingSmartObj,
	WSP_TargetIsDead,
	WSP_Action,
	WSP_HasItem,
	WSP_ItemEquipped,

	WSP_Count
};

#define EWSPropAssertBounds(Key) n_assert(Key > WSP_Invalid && Key < WSP_Count)

//???template with size param or enum param?
class CWorldState
{
private:

	Data::CData Props[WSP_Count];

public:

	//!!!getdiff etc

	void				SetProp(EWSProp Key, const Data::CData& Value);
	void				SetPropFrom(EWSProp Key, const CWorldState& Src);
	const Data::CData&	GetProp(EWSProp Key) const { return Props[Key]; }
	const Data::CData&	GetProp(int Key) const { EWSPropAssertBounds(Key); return Props[Key]; }
	bool				IsPropSet(EWSProp Key) const { EWSPropAssertBounds(Key); return Props[Key].IsValid(); }
	int					GetDiffCount(const CWorldState& Other) const;
};
//---------------------------------------------------------------------

inline void CWorldState::SetProp(EWSProp Key, const Data::CData& Value)
{
	//EWSPropAssertBounds(Key);
	Props[Key] = Value;
}
//---------------------------------------------------------------------

inline void CWorldState::SetPropFrom(EWSProp Key, const CWorldState& Src)
{
	//EWSPropAssertBounds(Key);
	Props[Key] = Src.Props[Key]; //???check IsPropSet?
}
//---------------------------------------------------------------------

}

DECLARE_TYPE(AI::EWSProp, 12);

#endif