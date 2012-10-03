#pragma once
#ifndef __IPG_ITEM_MANAGER_H__
#define __IPG_ITEM_MANAGER_H__

#include <Core/RefCounted.h>
#include <util/HashTable.h>
#include <Events/Events.h>
#include <DB/Dataset.h>
#include "ItemTpl.h"

// Item manager loads and keeps track of item templates, assists in saving & loading
// item-related stuff. Works like factory for CItemTpl type-specific derived classes.

namespace Data
{
	class CParams;
}

namespace Items
{
using namespace Data;

#define ItemMgr Items::CItemManager::Instance()

class CItemManager: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CItemManager);

private:

	static CItemManager*	Singleton;
	static int				LargestItemStackID;

	HashTable<CStrID, Ptr<CItemTpl>>	ItemTplRegistry;

	DB::PDataset						DSInv;
	DB::PDataset						DSEquip;
	nString								LevelInQuery;
	int									InitialVTRowCount;

	DECLARE_EVENT_HANDLER(OnSaveBefore, OnSaveBefore);
	DECLARE_EVENT_HANDLER(OnSaveAfter, OnSaveAfter);
	DECLARE_EVENT_HANDLER(OnLoadBefore, OnLoadBefore);
	DECLARE_EVENT_HANDLER(OnLoadAfter, OnLoadAfter);
	DECLARE_EVENT_HANDLER(OnGameDBClose, OnGameDBClose);

public:

	CItemManager();
	virtual ~CItemManager();

	static CItemManager* Instance() { n_assert(Singleton); return Singleton; }

	PItemTpl		CreateItemTpl(CStrID ID, const CParams& Params); //???type from params?
	PItemTpl		GetItemTpl(CStrID ID);
	DB::CDataset*	GetInventoriesDataset() const { return DSInv.get_unsafe(); }
	int				GetInventoriesRowCount() const { return InitialVTRowCount; }
	DB::CDataset*	GetEquipmentDataset() const { return DSEquip.get_unsafe(); }
	int				NewItemStackID() const { return ++LargestItemStackID; }
};

RegisterFactory(CItemManager);

}

#endif
