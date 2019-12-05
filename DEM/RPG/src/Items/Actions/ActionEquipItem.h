#pragma once
#include <AI/Behaviour/Action.h>
#include <Data/StringID.h>

// EquipItem action switches actor between moving in certain radius and standing still,
// sometimes facing random directions.

namespace AI
{

class CActionEquipItem: public CAction
{
	FACTORY_CLASS_DECL;

protected:

	CStrID Item;
	CStrID Slot;
	//???count?

public:

	void				Init(CStrID ItemID, CStrID SlotID = CStrID::Empty) { Item = ItemID; Slot = SlotID; }
	virtual bool		Activate(CActor* pActor);

	//virtual void		GetDebugString(std::string& Out) const { Out.Format("%s(%s, %s)", GetClassName().CStr(), Item.CStr(), Slot.CStr()); }
};

typedef Ptr<CActionEquipItem> PActionEquipItem;

}
