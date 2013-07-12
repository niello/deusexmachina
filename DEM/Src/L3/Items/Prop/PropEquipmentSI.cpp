#include "PropEquipment.h"

#include <Items/Item.h>
#include <Scripting/ScriptServer.h>
#include <Scripting/EntityScriptObject.h>
#include <Game/EntityManager.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
};

namespace Prop
{

using namespace Scripting;

int CPropEquipment_GetEquippedItemID(lua_State* l)
{
	//args: EntityScriptObject's this table, Slot ID
	SETUP_ENT_SI_ARGS(2);

	CDict<CStrID, CPropEquipment::CSlot>& Slots = This->GetEntity()->GetProperty<CPropEquipment>()->Slots;
	int Idx = Slots.FindIndex(CStrID(lua_tostring(l, 2)));
	if (Idx == INVALID_INDEX) lua_pushnil(l);
	else
	{
		CItemStack* pStack = Slots.ValueAt(Idx).pStack;
		if (pStack) lua_pushstring(l, pStack->GetItemID().CStr());
		else lua_pushnil(l);
	}

	return 1;
}
//---------------------------------------------------------------------

void CPropEquipment::ExposeSI()
{
	CPropInventory::ExposeSI();
	ScriptSrv->ExportCFunction("GetEquippedItemID", CPropEquipment_GetEquippedItemID);
}
//---------------------------------------------------------------------

} // namespace Prop