#pragma once
#ifndef __DEM_L3_ITEM_ATTRS_H__
#define __DEM_L3_ITEM_ATTRS_H__

#include <DB/AttrID.h>

namespace Attr
{
	DeclareAttr(ID);
	DeclareStrID(ItemOwner);
	DeclareStrID(ItemTplID);
	DeclareInt(ItemInstID);
	DeclareInt(ItemCount);
	DeclareStrID(EquipSlotID);
	DeclareInt(EquipCount);
	DeclareInt(LargestItemStackID);
};

#endif