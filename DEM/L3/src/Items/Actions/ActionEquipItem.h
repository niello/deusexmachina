#pragma once
#ifndef __DEM_L3_AI_ACTION_EQUIP_ITEM_H__
#define __DEM_L3_AI_ACTION_EQUIP_ITEM_H__

#include <AI/Behaviour/Action.h>
#include <Data/StringID.h>

// EquipItem action switches actor between moving in certain radius and standing still,
// sometimes facing random directions.

namespace AI
{

class CActionEquipItem: public CAction
{
	__DeclareClass(CActionEquipItem);

protected:

	CStrID Item;
	CStrID Slot;
	//???count?

public:

	void				Init(CStrID ItemID, CStrID SlotID = CStrID::Empty) { Item = ItemID; Slot = SlotID; }
	virtual bool		Activate(CActor* pActor);

	virtual void		GetDebugString(CString& Out) const { Out.Format("%s(%s, %s)", GetClassName().CStr(), Item.CStr(), Slot.CStr()); }
};

typedef Ptr<CActionEquipItem> PActionEquipItem;

}

#endif