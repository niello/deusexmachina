#pragma once
#include <Items/Prop/PropInventory.h>
#include <Data/Dictionary.h>

// Extends basic inventory with set of slots, accepting some volume/count of items of certain types
// and allowing characters to use these items and gain their effects.

namespace Prop
{

class CPropEquipment: public CPropInventory
{
	FACTORY_CLASS_DECL;

protected:

	virtual void EnableSI(class CPropScriptable& Prop);
	virtual void DisableSI(class CPropScriptable& Prop);

public:

	struct CSlot
	{
		CStrID				Type; //!!!need map: slot type => item types. Now 1:1.
		Items::CItemStack*	pStack;
		U16					Count;
		//WORD				MaxCount;

		CSlot(): pStack(nullptr), Count(0) {}
	};

	CDict<CStrID, CSlot> Slots; //???to protected?

	CPropEquipment();

	bool Equip(CStrID Slot, Items::CItemStack* pStack, U16 Count = 1); //???for 0 or -1 count as many as possible?
	void Unequip(CStrID SlotID);
};

}
