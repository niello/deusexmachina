#pragma once
#ifndef __DEM_L3_ITEM_MANAGER_H__
#define __DEM_L3_ITEM_MANAGER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <Data/HashTable.h>
#include <Items/ItemTpl.h>

// Item manager loads and keeps track of item templates. Works like factory for CItemTpl
// type-specific derived classes.

namespace Data
{
	class CParams;
}

namespace Items
{
#define ItemMgr Items::CItemManager::Instance()

class CItemManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CItemManager);

private:

	CHashTable<CStrID, PItemTpl> ItemTplRegistry;

public:

	CItemManager() { __ConstructSingleton; }
	~CItemManager() { __DestructSingleton; }

	PItemTpl CreateItemTpl(CStrID ID, const Data::CParams& Params); //???type from params?
	PItemTpl GetItemTpl(CStrID ID);
};

}

#endif
