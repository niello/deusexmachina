#include "ItemManager.h"

#include <Items/ItemTpl.h>
#include <Items/ItemAttrs.h>
#include <Loading/LoaderServer.h>
#include <Data/Params.h>
#include <Data/DataServer.h>
#include <Events/EventManager.h>
#include <DB/Database.h>

extern const nString CommaFrag;

namespace Items
{
ImplementRTTI(Items::CItemManager, Core::CRefCounted);
ImplementFactory(Items::CItemManager);

CItemManager* CItemManager::Singleton = NULL;
int CItemManager::LargestItemStackID = 0;

CItemManager::CItemManager(): InitialVTRowCount(0)
{
	n_assert(!Singleton);
	Singleton = this;
	DataSrv->SetAssign("items", "game:items");
	if (LoaderSrv->HasGlobal(Attr::LargestItemStackID))
		LargestItemStackID = LoaderSrv->GetGlobal<int>(Attr::LargestItemStackID);

	// Unsubscribed in destructor automatically
	SUBSCRIBE_PEVENT(OnSaveBefore, CItemManager, OnSaveBefore);
	SUBSCRIBE_PEVENT(OnSaveAfter, CItemManager, OnSaveAfter);
	SUBSCRIBE_PEVENT(OnLoadBefore, CItemManager, OnLoadBefore);
	SUBSCRIBE_PEVENT(OnLoadAfter, CItemManager, OnLoadAfter);
	SUBSCRIBE_PEVENT(OnGameDBClose, CItemManager, OnGameDBClose);
}
//---------------------------------------------------------------------

CItemManager::~CItemManager()
{
	n_assert(Singleton);
	Singleton = NULL;
}
//---------------------------------------------------------------------

Ptr<CItemTpl> CItemManager::CreateItemTpl(CStrID ID, const CParams& Params)
{
	Ptr<CItemTpl> Tpl =
		(CItemTpl*)CoreFct->Create(nString("Items::CItemTpl") + Params.Get<nString>(CStrID("Type"), NULL));
	n_assert(Tpl.isvalid());
	Tpl->Init(ID, Params);
	return Tpl;
}
//---------------------------------------------------------------------

Ptr<CItemTpl> CItemManager::GetItemTpl(CStrID ID)
{
	Ptr<CItemTpl> Tpl;
	if (ItemTplRegistry.Get(ID, Tpl)) return Tpl;
	else
	{
		PParams HRD = DataSrv->LoadPRM(nString("items:") + ID.CStr() + ".prm", false);
		if (HRD.isvalid())
		{
			Tpl = CreateItemTpl(ID, *HRD);
			n_assert(Tpl.isvalid());
			ItemTplRegistry.Add(ID, Tpl);
			return Tpl;
		}
		else return Ptr<CItemTpl>();
	}
}
//---------------------------------------------------------------------

bool CItemManager::OnSaveBefore(const Events::CEventBase& Event)
{
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));

	if (!DSInv.isvalid())
	{
		int TableIdx = pDB->FindTableIndex("Inventories");
		if (TableIdx == INVALID_INDEX)
		{
			DB::PTable Tbl = DB::CTable::Create();
			Tbl->SetName("Inventories");
			Tbl->AddColumn(DB::CColumn(Attr::ID, DB::CColumn::Primary)); // For equipment records
			Tbl->AddColumn(DB::CColumn(Attr::ItemOwner, DB::CColumn::Indexed));
			Tbl->AddColumn(Attr::ItemTplID);
			Tbl->AddColumn(Attr::ItemInstID);
			Tbl->AddColumn(Attr::ItemCount);
			pDB->AddTable(Tbl);
			DSInv = Tbl->CreateDataset();
		}
		else DSInv = pDB->GetTable(TableIdx)->CreateDataset();

		//LevelInQuery = nString::Empty;
		n_assert(LevelInQuery.IsEmpty());
	}

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

	if (!DSEquip.isvalid())
	{
		DB::PTable Tbl;
		int TableIdx = pDB->FindTableIndex("Equipment");
		if (TableIdx == INVALID_INDEX)
		{
			DB::PTable Tbl = DB::CTable::Create();
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

	OK;
}
//---------------------------------------------------------------------

bool CItemManager::OnSaveAfter(const Events::CEventBase& Event)
{
	DSInv->CommitChanges();
	DSInv->Clear();
	InitialVTRowCount = 0;

	DSEquip->CommitChanges();
	DSEquip->Clear();

	// Commit instance VTs
	// Kill instance VTs

	LoaderSrv->SetGlobal<int>(Attr::LargestItemStackID, LargestItemStackID);

	OK;
}
//---------------------------------------------------------------------

bool CItemManager::OnLoadBefore(const Events::CEventBase& Event)
{
	DB::CDatabase* pDB = (DB::CDatabase*)((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("DB"));

	if (!DSInv.isvalid())
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
		if (!DSEquip.isvalid())
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

	OK;
}
//---------------------------------------------------------------------

bool CItemManager::OnLoadAfter(const Events::CEventBase& Event)
{
	if (DSInv.isvalid() && DSInv->GetRowCount() > 0)
	{
		DSInv->Clear();
		if (DSEquip->GetRowCount() > 0) DSEquip->Clear();
	}
	
	// Kill instance VTs
	
	OK;
}
//---------------------------------------------------------------------

bool CItemManager::OnGameDBClose(const Events::CEventBase& Event)
{
	DSInv = NULL;
	DSEquip = NULL;
	LevelInQuery = nString::Empty;
	OK;
}
//---------------------------------------------------------------------

} //namespace Items