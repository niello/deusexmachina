#include "ItemAttrs.h"

#include <DB/DBServer.h>

namespace Attr
{
	DefineStrID(ItemOwner);
	DefineStrID(ItemTplID);
	DefineInt(ItemInstID);
	DefineInt(ItemCount);
	DefineStrID(EquipSlotID);
	DefineInt(EquipCount);
	DefineInt(LargestItemStackID);
};

BEGIN_ATTRS_REGISTRATION(ItemAttrs)
	RegisterStrID(ItemOwner, ReadOnly);
	RegisterStrID(ItemTplID, ReadWrite);
	RegisterIntWithDefault(ItemInstID, ReadWrite, -1);
	RegisterIntWithDefault(ItemCount, ReadWrite, 1);
	RegisterStrID(EquipSlotID, ReadWrite);
	RegisterIntWithDefault(EquipCount, ReadWrite, 1);
	RegisterIntWithDefault(LargestItemStackID, ReadWrite, 0);
END_ATTRS_REGISTRATION