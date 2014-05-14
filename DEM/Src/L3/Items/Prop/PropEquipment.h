#pragma once
#ifndef __DEM_L3_PROP_EQUIPMENT_H__
#define __DEM_L3_PROP_EQUIPMENT_H__

#include <Items/Prop/PropInventory.h>
#include <Data/Dictionary.h>

// Extends basic inventory with set of slots, accepting some volume/count of items of certain types
// and allowing characters to use these items and gain their effects.

namespace Prop
{

class CPropEquipment: public CPropInventory
{
	__DeclareClass(CPropEquipment);

protected:

	virtual void EnableSI(class CPropScriptable& Prop);
	virtual void DisableSI(class CPropScriptable& Prop);

public:

	struct CSlot
	{
		CStrID		Type; //!!!need map: slot type => item types. Now 1:1.
		CItemStack*	pStack;
		WORD		Count;
		//WORD		MaxCount;

		CSlot(): pStack(NULL), Count(0) {}
	};

	CDict<CStrID, CSlot> Slots; //???to protected?

	CPropEquipment();

	bool Equip(CStrID Slot, CItemStack* pStack, WORD Count = 1); //???for 0 or -1 count as many as possible?
	void Unequip(CStrID SlotID);
};

}

#endif