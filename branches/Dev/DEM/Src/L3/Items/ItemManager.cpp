#include "ItemManager.h"

#include <Items/ItemTpl.h>
#include <Data/Params.h>
#include <IO/IOServer.h>
#include <Data/DataServer.h>
#include <Events/EventManager.h>

//BEGIN_ATTRS_REGISTRATION(ItemAttrs)
//	RegisterStrID(ItemOwner, ReadOnly);
//	RegisterStrID(ItemTplID, ReadWrite);
//	RegisterIntWithDefault(ItemCount, ReadWrite, 1);
//	RegisterStrID(EquipSlotID, ReadWrite);
//	RegisterIntWithDefault(EquipCount, ReadWrite, 1);
//	RegisterIntWithDefault(LargestItemStackID, ReadWrite, 0);
//END_ATTRS_REGISTRATION

extern const nString CommaFrag;

namespace Items
{
__ImplementClassNoFactory(Items::CItemManager, Core::CRefCounted);
__ImplementSingleton(Items::CItemManager);

int CItemManager::LargestItemStackID = 0;

CItemManager::CItemManager(): InitialVTRowCount(0)
{
	__ConstructSingleton;

	IOSrv->SetAssign("items", "game:items");
	//if (LoaderSrv->HasGlobal(Attr::LargestItemStackID))
	//	LargestItemStackID = LoaderSrv->GetGlobal<int>(Attr::LargestItemStackID);

	// Unsubscribed in destructor automatically
	SUBSCRIBE_PEVENT(OnSaveBefore, CItemManager, OnSaveBefore);
	SUBSCRIBE_PEVENT(OnSaveAfter, CItemManager, OnSaveAfter);
	SUBSCRIBE_PEVENT(OnLoadBefore, CItemManager, OnLoadBefore);
	SUBSCRIBE_PEVENT(OnLoadAfter, CItemManager, OnLoadAfter);
}
//---------------------------------------------------------------------

CItemManager::~CItemManager()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

PItemTpl CItemManager::CreateItemTpl(CStrID ID, const CParams& Params)
{
	PItemTpl Tpl =
		(CItemTpl*)Factory->Create(nString("Items::CItemTpl") + Params.Get<nString>(CStrID("Type"), NULL));
	n_assert(Tpl.IsValid());
	Tpl->Init(ID, Params);
	return Tpl;
}
//---------------------------------------------------------------------

PItemTpl CItemManager::GetItemTpl(CStrID ID)
{
	PItemTpl Tpl;
	if (ItemTplRegistry.Get(ID, Tpl)) return Tpl;
	else
	{
		PParams HRD = DataSrv->LoadPRM(nString("items:") + ID.CStr() + ".prm", false);
		if (HRD.IsValid())
		{
			Tpl = CreateItemTpl(ID, *HRD);
			n_assert(Tpl.IsValid());
			ItemTplRegistry.Add(ID, Tpl);
			return Tpl;
		}
		else return PItemTpl();
	}
}
//---------------------------------------------------------------------

bool CItemManager::OnSaveBefore(const Events::CEventBase& Event)
{
	/*
	nString CurrLevel = LoaderSrv->GetCurrentLevel();
	n_assert(CurrLevel.IsValid());
	if (LevelInQuery != CurrLevel)
	{
		DSInv->SetSelectSQL("SELECT * FROM Inventories WHERE ItemOwner IN "
							" (SELECT GUID FROM InstPlrChar " //WHERE LevelID=?
							"  UNION ALL SELECT GUID FROM InstNPC WHERE LevelID='" + CurrLevel + "' "
							"  UNION ALL SELECT GUID FROM InstContainer WHERE LevelID='" + CurrLevel + "') "
							"ORDER BY ItemOwner");
		LevelInQuery = CurrLevel;
	}

	DSInv->PerformQuery();

	InitialVTRowCount = DSInv->GetRowCount();

	if (!DSEquip.IsValid())
	{
		DB::PTable Tbl;
		int TableIdx = pDB->FindTableIndex("Equipment");
		if (TableIdx == INVALID_INDEX)
		{
			DB::PTable Tbl = DB::CTable::CreateInstance();
			Tbl->SetName("Equipment");
			Tbl->AddColumn(DB::CColumn(Attr::ID, DB::CColumn::Primary)); // Stack ID
			Tbl->AddColumn(Attr::EquipSlotID);
			Tbl->AddColumn(Attr::EquipCount);
			pDB->AddTable(Tbl);
			DSEquip = Tbl->CreateDataset();
		}
		else DSEquip = pDB->GetTable(TableIdx)->CreateDataset();
		DSEquip->AddColumnsFromTable();
	}

	if (InitialVTRowCount)
	{
		nString SQL;
		//SQL.Reserve(2048);
		SQL.Format("ID IN (%d", DSInv->GetValueTable()->Get<int>(0, 0));
		for (int i = 1; i < DSInv->GetRowCount(); i++)
		{
			SQL += CommaFrag;
			SQL += nString::FromInt(DSInv->GetValueTable()->Get<int>(0, i));
		}
		SQL += ")";

		DSEquip->DeleteWhere(SQL);
	}

	// request item instance tables (whole? on demand?)
*/

	OK;
}
//---------------------------------------------------------------------

bool CItemManager::OnSaveAfter(const Events::CEventBase& Event)
{
	/*
	DSInv->CommitChanges();
	DSInv->Clear();
	InitialVTRowCount = 0;

	DSEquip->CommitChanges();
	DSEquip->Clear();

	// Commit instance VTs
	// Kill instance VTs

	LoaderSrv->SetGlobal<int>(Attr::LargestItemStackID, LargestItemStackID);
*/

	OK;
}
//---------------------------------------------------------------------

bool CItemManager::OnLoadBefore(const Events::CEventBase& Event)
{
	/*
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));

	if (!DSInv.IsValid())
	{
		int TableIdx = pDB->FindTableIndex("Inventories");
		if (TableIdx == INVALID_INDEX)
		{
			LargestItemStackID = 0;
			OK;
		}

		DSInv = pDB->GetTable(TableIdx)->CreateDataset();

		//LevelInQuery = nString::Empty;
		n_assert(LevelInQuery.IsEmpty());
	}
		
	LargestItemStackID = LoaderSrv->GetGlobal<int>(Attr::LargestItemStackID);
	
	nString CurrLevel = LoaderSrv->GetCurrentLevel();
	if (CurrLevel.IsEmpty()) OK;
	if (LevelInQuery != CurrLevel)
	{
		DSInv->SetSelectSQL("SELECT * FROM Inventories WHERE ItemOwner IN "
							" (SELECT GUID FROM InstPlrChar " //WHERE LevelID=?
							"  UNION ALL SELECT GUID FROM InstNPC WHERE LevelID='" + CurrLevel + "' "
							"  UNION ALL SELECT GUID FROM InstContainer WHERE LevelID='" + CurrLevel + "') "
							"ORDER BY ItemOwner");
		LevelInQuery = CurrLevel;
	}

	DSInv->PerformQuery();

	if (DSInv->GetRowCount())
	{
		if (!DSEquip.IsValid())
		{
			DB::PTable Tbl;
			int TableIdx = pDB->FindTableIndex("Equipment");
			if (TableIdx == INVALID_INDEX) OK;
			DSEquip = pDB->GetTable(TableIdx)->CreateDataset();
		}

		nString SQL;
		//SQL.Reserve(2048);
		SQL.Format("SELECT * FROM Equipment WHERE ID IN (%d", DSInv->GetValueTable()->Get<int>(0, 0));
		for (int i = 1; i < DSInv->GetRowCount(); i++)
		{
			SQL += CommaFrag;
			SQL += nString::FromInt(DSInv->GetValueTable()->Get<int>(0, i));
		}
		SQL += ") ORDER BY ID";

		DSEquip->SetSelectSQL(SQL);
		DSEquip->PerformQuery();
	}

	// May be needed not only for Invs but for item entities too
	// request item instance tables (whole? on demand?)
*/
	OK;
}
//---------------------------------------------------------------------

bool CItemManager::OnLoadAfter(const Events::CEventBase& Event)
{
	//if (DSInv.IsValid() && DSInv->GetRowCount() > 0)
	//{
	//	DSInv->Clear();
	//	if (DSEquip->GetRowCount() > 0) DSEquip->Clear();
	//}
	
	// Kill instance VTs
	
	OK;
}
//---------------------------------------------------------------------

} //namespace Items