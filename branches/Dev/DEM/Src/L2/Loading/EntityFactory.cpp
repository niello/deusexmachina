#include "EntityFactory.h"

#include <Game/Entity.h>
#include <Loading/LoaderServer.h>
#include <Events/EventManager.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <DB/Database.h>

const nString StrTpl("Tpl");
const nString StrInst("Inst");
const nString StrPropPrefix("Properties::");

namespace Attr
{
	DeclareAttr(LevelID);
	DeclareAttr(Transform);
}

namespace Loading
{
using namespace Game;

CEntityFactory::CEntityFactory(): PropertyMeta(CPropertyInfo(), 64)
{
}
//---------------------------------------------------------------------

CEntityFactory::~CEntityFactory()
{
}
//---------------------------------------------------------------------

void CEntityFactory::Init()
{
	n_assert(!_IsActive);
	_IsActive = true;
	
	Categories.Clear();

	PParams P = DataSrv->LoadHRD("data:tables/EntityCats.hrd");
	if (!P.isvalid()) n_error("Loading::CEntityFactory: Error loading EntityCats.hrd!");

	for (int i = 0; i < P->GetCount(); i++)
		CreateEntityCat(P->Get(i).GetName(), P->Get(i).GetValue<PParams>());

	SUBSCRIBE_PEVENT(OnStaticDBClose, CEntityFactory, OnStaticDBClose);
	SUBSCRIBE_PEVENT(OnGameDBClose, CEntityFactory, OnGameDBClose);
}
//---------------------------------------------------------------------

void CEntityFactory::Release()
{
	n_assert(_IsActive);
	UNSUBSCRIBE_EVENT(OnStaticDBClose);
	UNSUBSCRIBE_EVENT(OnGameDBClose);
	RemoveAllLoaders();
	Categories.Clear();
	_IsActive = false;
}
//---------------------------------------------------------------------

CEntity* CEntityFactory::CreateEntityByClassName(const nString& CppClassName) const
{
	if (CppClassName == "Entity") return CEntity::Create();
	else
	{
		n_error("Loading::CEntityFactory::CreateEntity(): unknown entity class name '%s'!", CppClassName.Get());
		return NULL;
	}
}
//---------------------------------------------------------------------

//Create an entity from its category name. The category name is looked
//up in the EntityCats.hrd file to check what properties must be attached
//to the entity. All required properties will be attached, and all
//attributes will be initialised in their default state.
//!!!!!tmp not const!
CEntity* CEntityFactory::CreateEntityByCategory(CStrID GUID, CStrID Category, EntityPool EntPool) //const
{
	int Idx = Categories.FindIndex(Category);
	if (Idx != INVALID_INDEX)
	{
		const CEntityCat& Cat = Categories.ValueAtIndex(Idx);
		n_assert(Cat.InstDataset.isvalid());
		
		CEntity* pEntity = CreateEntityByClassName(Cat.CppClass);
		pEntity->SetLive(EntPool == LivePool);
		pEntity->SetAttrTable(Cat.InstDataset->GetValueTable());
		pEntity->SetAttrTableRowIndex(Cat.InstDataset->GetValueTable()->AddRow());
		pEntity->SetCategory(Category);
		pEntity->SetUniqueID(GUID);

		for (int i = 0; i < Cat.Properties.Size(); i++)
			AttachProperty(*pEntity, Cat.Properties[i]);

		return pEntity;
	}
	else
	{
		n_error("Loading::CEntityFactory::CreateEntityByCategory(%s): category not found in EntityCats.hrd!",
		Category.CStr());
		return NULL;
	}
}
//---------------------------------------------------------------------

CEntity* CEntityFactory::CreateEntityByTemplate(CStrID GUID, CStrID Category, CStrID TplName, EntityPool EntPool)
{
	n_assert(Category.IsValid());
	n_assert(TplName.IsValid());

	const CEntityCat& Cat = Categories[Category];
	
	int TplIdx = FindTemplate(TplName, Cat);
	if (TplIdx == INVALID_INDEX) return NULL;

	CEntity* pEntity = CreateEntityByClassName(Cat.CppClass);
	pEntity->SetLive(EntPool == LivePool);
	pEntity->SetAttrTable(Cat.InstDataset->GetValueTable());
	pEntity->SetAttrTableRowIndex(Cat.InstDataset->GetValueTable()->CopyExtRow(Cat.TplDataset->GetValueTable(), TplIdx, true));
	pEntity->SetCategory(Category);
	pEntity->SetUniqueID(GUID);

	for (int i = 0; i < Cat.Properties.Size(); i++)
		AttachProperty(*pEntity, Cat.Properties[i]);

	return pEntity;
}
//---------------------------------------------------------------------

// Create a entity as a clone of a existing one. A new GUID will be assigned.
CEntity* CEntityFactory::CreateEntityByEntity(CStrID GUID, CEntity* pTplEntity, EntityPool EntPool) const
{
	n_assert(pTplEntity);
	n_assert(pTplEntity->GetCategory().IsValid());
	n_assert(pTplEntity->HasAttr(Attr::GUID) && pTplEntity->Get<CStrID>(Attr::GUID).IsValid());

	//???!!!tpl name arg as CStrID, not nString?!
	CEntity* pEntity = NULL;// CreateEntityByTemplate(GUID, pTplEntity->GetCategory().CStr(),//???CStrID arg?
		//pTplEntity->Get<CStrID>(Attr::GUID).CStr());
	n_assert(pEntity);

	//const nArray<DB::CAttr>& TplAttrs = pTplEntity->GetAttrs();
	//for (int i = 0; i < TplAttrs.Size(); i++)
	//	if (TplAttrs[i].GetAttrID() != Attr::GUID)
	//		pEntity->SetAttr(TplAttrs[i]);

	return pEntity;
}
//---------------------------------------------------------------------

/**
    This will 'load' a new entity from the world database (AKA making the
    entity 'live') and place it in the given entity pool (Live or Sleeping).
    This will create a new entity, attach properties as described by
    EntityCats.hrd, and update the entity attributes from the database.
    Changes to attributes can later be written back to the
    database by calling the CEntity::Save() method.

    NOTE: this method will not call the CEntity::OnLoad() method, which may be
    required to finally initialize the entity. The OnLoad() method expects
    that all other Entities in the level have already been loaded, so this
    must be done after loading in a separate pass.

    FIXME: This method does 2 complete queries on the database!!
*/
CEntity* CEntityFactory::CreateEntityByKeyAttr(DB::CAttrID AttrID, const Data::CData& Value, EntityPool EntPool) const
{
	//Ptr<DB::Query> DBQuery = DBSrv->CreateQuery();
	//DBQuery->SetTableName("_Entities");
	//DBQuery->AddWhereAttr(Key);
	//DBQuery->AddWhereAttr(DB::CAttr(Attr::_Type, nString("INSTANCE")));
	//DBQuery->AddResultAttr(Attr::_Category);
	//DBQuery->AddResultAttr(Attr::GUID);
	//DBQuery->BuildSelectStatement();
	//if (DBQuery->Execute())
	//{
	//	if (DBQuery->GetRowCount() != 1)
	//	{
	//		n_error("Loading::CEntityFactory::CreateEntityByKeyAttr(): %s Key '%s=%s' in world database!",
	//			DBQuery->GetRowCount() ? "more then one entry with" : "no", Key.GetName().Get(), Key.AsString().Get());
	//		return NULL;
	//	}

	//	//!!!!!!!!!!!!!!GetCategory!
	//	//!!!!!!!!!!!
	//	CEntity* pEntity = CreateEntityByCategory(DBQuery->Get<nString>(Attr::GUID, 0),
	//											  DBQuery->Get<nString>(Attr::_Category, 0),
	//											  EntPool);
	//	pEntity->LoadAttributesFromDatabase();
	//	return pEntity;
	//}

	//n_error("Loading::CEntityFactory::CreateEntityByKeyAttr(): failed to load entity with Key '%s=%s' from world database!", Key.GetName().Get(), Key.AsString().Get());
	return NULL;
}
//---------------------------------------------------------------------

/**
    This will 'load' a new Entities from the world database (AKA making the
    entity 'live') and place it in the given entity pool (Live or Sleeping).
    This will create new Entities, attach properties as described by
    EntityCats.hrd, and update the Entities attributes from the database.
    Changes to attributes can later be written back to the
    database by calling the CEntity::Save() method.

    NOTE: this method will not call the CEntity::OnLoad() method, which may be
    required to finally initialize the entity. The OnLoad() method expects
    that all other Entities in the level have already been loaded, so this
    must be done after loading in a separate pass.

    FIXME: This method does 1 + numEnities complete queries on the database!!
*/
/*
nArray<CEntity*> CEntityFactory::CreateEntitiesByKeyAttrs(const nArray<DB::CAttr>& Keys,
														   const nArray<PEntity>& FilteredEnts,
														   EntityPool EntPool,
														   bool FailOnDBError) const
{
	nArray<CEntity*> Entities;

	//Ptr<DB::Query> DBQuery = DBSrv->CreateQuery();
	//DBQuery->SetTableName("_Entities");
	//DBQuery->AddWhereAttr(DB::CAttr(Attr::_Type, nString("INSTANCE")));

	//for (int i = 0; i < Keys.Size(); i++) DBQuery->AddWhereAttr(Keys[i]);
	//for (int i = 0; i < FilteredEnts.Size(); i++) DBQuery->AddWhereAttr(FilteredEnts[i]->GetAttr(Attr::GUID), true);

	//DBQuery->AddResultAttr(Attr::_Category);
	//DBQuery->AddResultAttr(Attr::GUID);
	//DBQuery->BuildSelectStatement();

	//if (DBQuery->Execute(FailOnDBError) && DBQuery->GetRowCount() > 0)
	//	for (int i = 0; i < DBQuery->GetRowCount(); i++)
	//	{
	//		CEntity* pEntity = CreateEntityByCategory(DBQuery->Get<nString>(Attr::GUID, i),
	//												  DBQuery->Get<nString>(Attr::_Category, i),
	//												  EntPool);
	//		pEntity->LoadAttributesFromDatabase();
	//		Entities.Append(pEntity);
	//	}

	return Entities;
}
//---------------------------------------------------------------------
*/

CEntity* CEntityFactory::CreateTmpEntity(CStrID GUID, CStrID Category, PValueTable Table, int Row) const
{
	CEntity* pEntity = CreateEntityByClassName("Entity");
	pEntity->SetCategory(Category); //???add to dummy table all attrs if category exists?
	pEntity->SetAttrTable(Table);
	pEntity->SetAttrTableRowIndex(Row);
	pEntity->SetUniqueID(GUID);
	pEntity->SetLive(true);
	return pEntity;
}
//---------------------------------------------------------------------

PEntity CEntityFactory::CreateEntityByCategory(CStrID Category, DB::CValueTable* pTable, int RowIdx) const
{
	int Idx = Categories.FindIndex(Category);
	if (Idx != INVALID_INDEX)
	{
		const CEntityCat& Cat = Categories.ValueAtIndex(Idx);
		PEntity pEntity = CreateEntityByClassName(Cat.CppClass);
		pEntity->SetLive(true);//EntPool == LivePool); //???layers now?

		pEntity->SetAttrTable(pTable);
		pEntity->SetAttrTableRowIndex(RowIdx);
		pEntity->SetUniqueIDFromAttrTable();
		pEntity->SetCategory(Category);

		for (int i = 0; i < Cat.Properties.Size(); i++)
			AttachProperty(*pEntity, Cat.Properties[i]);
		
		return pEntity;
	}
	else
	{
		n_error("CEntityFactory::CreateEntityByCategory(%s): category not found in EntityCats.hrd!",
		Category.CStr());
		return NULL;
	}
}
//---------------------------------------------------------------------

CProperty* CEntityFactory::AttachProperty(CEntity& Entity, const nString& TypeName) const
{
	CPropertyInfo PropInfo;
	nString Type = TypeName;
	if (!PropertyMeta.Get(Type.Get(), PropInfo))
	{
		Type = StrPropPrefix + TypeName;
		if (!PropertyMeta.Get(Type.Get(), PropInfo))
			n_error("No such property \"%s\"", TypeName.Get());
	}

	n_assert(PropInfo.pStorage);

	if (PropInfo.ActivePools & Entity.GetEntityPool())
	{
		PProperty Prop;
		if (!PropInfo.pStorage->Get(Entity.GetUniqueID(), Prop))
		{
			Prop = (CProperty*)CoreFct->Create(Type);
			PropInfo.pStorage->Add(Entity.GetUniqueID(), Prop);
			Prop->SetEntity(&Entity);
		}
		return Prop.get_unsafe();
	}

	return NULL;
}
//---------------------------------------------------------------------

void CEntityFactory::DetachProperty(CEntity& Entity, const nString& TypeName) const
{
	CPropertyInfo PropInfo;

	nString FullType = StrPropPrefix + TypeName;
	if (!PropertyMeta.Get(FullType.Get(), PropInfo))
		if (!PropertyMeta.Get(TypeName.Get(), PropInfo))
			n_error("No such property \"%s\"", TypeName.Get()); //???error?

	n_assert(PropInfo.pStorage);

#ifdef _DEBUG
	if (!PropInfo.pStorage->Contains(Entity.GetUniqueID()))
		n_error("CEntity::RemoveProperty: CProperty '%s' does not exist on entity!", TypeName.Get());
#endif

	(*PropInfo.pStorage)[Entity.GetUniqueID()]->ClearEntity(); //???need or there is always destructor?
	PropInfo.pStorage->Erase(Entity.GetUniqueID());
}
//---------------------------------------------------------------------

void CEntityFactory::DetachAllProperties(CEntity& Entity) const
{
	for (int i = 0; i < PropStorage.Size(); i++)
		PropStorage[i]->Erase(Entity.GetUniqueID());
}
//---------------------------------------------------------------------

void CEntityFactory::CreateEntityCat(CStrID Name, const PParams& Desc) //???need ref of smart ptr?
{
	CEntityCat New;

	New.CppClass = Desc->Get<nString>(CStrID("CppClass"));
	New.TplTableName = Desc->Get<nString>(CStrID("TplTableName"), StrTpl + Name.CStr());
	New.InstTableName = Desc->Get<nString>(CStrID("InstTableName"), StrInst + Name.CStr());
	New.AllowDefaultLoader = true;
	
	CDataArray& Props = *Desc->Get<PDataArray>(CStrID("Props"));
	for (int i = 0; i < Props.Size(); ++i)
	{
		const nString& PropName = Props[i].GetValue<nString>();
		New.Properties.Append(PropName);
		PProperty Prop = (CProperty*)CoreFct->Create(StrPropPrefix + PropName);
		Prop->GetAttributes(New.Attrs);
	}

	Categories.Add(Name, New);
}
//---------------------------------------------------------------------

const char* CEntityFactory::GetTemplateID(CStrID CatID, int Idx) const
{
	return Categories[CatID].TplDataset->GetValueTable()->Get<CStrID>(Attr::GUID, Idx).CStr();
}
//---------------------------------------------------------------------

void CEntityFactory::CreateNewEntityCat(CStrID Name, const Data::PParams& Desc, bool CreateInstDataset)
{
	CreateEntityCat(Name, Desc);

	CEntityCat& Cat = Categories[Name];

	PParams P = DataSrv->LoadHRD("data:tables/EntityCats.hrd");
	P->Set(Name, Desc);
	DataSrv->SaveHRD("data:tables/EntityCats.hrd", P);

	// Create Tpl table

	if (LoaderSrv->GetStaticDB()->HasTable(Cat.TplTableName))
		LoaderSrv->GetStaticDB()->DeleteTable(Cat.TplTableName);
	
	PTable TplTable = DB::CTable::Create();
	TplTable->SetName(Cat.TplTableName);
	TplTable->AddColumn(CColumn(Attr::GUID, CColumn::Primary));
	// All other tpl fields are added on demand by editor
	// Template creation is available only in editor

	LoaderSrv->GetStaticDB()->AddTable(TplTable);

	Cat.TplDataset = TplTable->CreateDataset();
	Cat.TplDataset->AddColumnsFromTable();

	// Create Inst table

	if (LoaderSrv->GetGameDB()->HasTable(Cat.InstTableName))
		LoaderSrv->GetGameDB()->DeleteTable(Cat.InstTableName);
	
	PTable InstTable = DB::CTable::Create();
	InstTable->SetName(Cat.InstTableName);
	InstTable->AddColumn(CColumn(Attr::GUID, CColumn::Primary));
	InstTable->AddColumn(CColumn(Attr::LevelID, CColumn::Indexed));
	for (nArray<CAttrID>::iterator It = Cat.Attrs.Begin(); It != Cat.Attrs.End(); It++)
		InstTable->AddColumn(*It);
	
	LoaderSrv->GetGameDB()->AddTable(InstTable);

	if (CreateInstDataset)
	{
		Cat.InstDataset = LoaderSrv->GetGameDB()->GetTable(Cat.InstTableName)->CreateDataset();
		Cat.InstDataset->AddColumnsFromTable();
	}
}
//---------------------------------------------------------------------

void CEntityFactory::LoadEntityTemplates()
{
	for (int i = 0; i < Categories.Size(); i++)
	{
		CEntityCat& Cat = Categories.ValueAtIndex(i);
		if (Cat.TplTableName.IsValid())
		{
			Cat.TplDataset = LoaderSrv->GetStaticDB()->GetTable(Cat.TplTableName)->CreateDataset();
			Cat.TplDataset->AddColumnsFromTable();
			Cat.TplDataset->PerformQuery();
		}
	}
}
//---------------------------------------------------------------------

DB::CDataset* CEntityFactory::GetTemplate(CStrID UID, CStrID Category, bool CreateIfNotExist)
{
	if (!Categories.Contains(Category)) return NULL;

	CEntityCat& Cat = Categories[Category];

	if (!Cat.TplDataset.isvalid()) return NULL;

	for (int i = 0; i < Cat.TplDataset->GetRowCount(); i++)
		if (Cat.TplDataset->GetValueTable()->IsRowValid(i))
		{
			Cat.TplDataset->SetRowIndex(i);
			if (Cat.TplDataset->Get<CStrID>(Attr::GUID) == UID) return Cat.TplDataset;
		}

	if (CreateIfNotExist)
	{
		Cat.TplDataset->AddRow();
		Cat.TplDataset->Set<CStrID>(Attr::GUID, UID);
		return Cat.TplDataset;
	}

	return NULL;
}
//---------------------------------------------------------------------

void CEntityFactory::DeleteTemplate(CStrID UID, CStrID Category)
{
	if (!Categories.Contains(Category)) return;

	CEntityCat& Cat = Categories[Category];

	if (Cat.TplDataset.isvalid())
		for (int i = 0; i < Cat.TplDataset->GetRowCount(); i++)
			if (Cat.TplDataset->GetValueTable()->IsRowValid(i))
				if (Cat.TplDataset->GetValueTable()->Get<CStrID>(Attr::GUID, i) == UID)
					Cat.TplDataset->GetValueTable()->DeleteRow(i);
}
//---------------------------------------------------------------------

int CEntityFactory::FindTemplate(CStrID UID, const CEntityCat& Cat)
{
	n_assert(Cat.TplDataset.isvalid());

	const DB::PValueTable VT = Cat.TplDataset->GetValueTable();
	int Col = VT->GetColumnIndex(Attr::GUID);

	for (int i = 0; i < VT->GetRowCount(); i++)
		if (VT->IsRowValid(i) && VT->Get<CStrID>(Col, i) == UID) return i;

	return -1;
}
//---------------------------------------------------------------------

void CEntityFactory::LoadEntityInstances(const nString& LevelName)
{
	bool IsLevelValid = LevelName.IsValid();

	for (int i = 0; i < Categories.Size(); i++)
	{
		CEntityCat& Cat = Categories.ValueAtIndex(i);
		if (Cat.InstTableName.IsValid())
		{
			Cat.InstDataset = LoaderSrv->GetGameDB()->GetTable(Cat.InstTableName)->CreateDataset();
			Cat.InstDataset->AddColumnsFromTable();

			PValueTable Values = Cat.InstDataset->GetValueTable();
			Values->BeginAddColumns();
			for (nArray<CAttrID>::iterator It = Cat.Attrs.Begin(); It != Cat.Attrs.End(); It++)
				Values->AddColumn(*It);
			Values->EndAddColumns();

			if (IsLevelValid)
			{
				Cat.InstDataset->SetWhereClause("LevelID='" + LevelName + "'");
				Cat.InstDataset->PerformQuery();

				Ptr<CEntityLoaderBase> Loader = Cat.Loader;
				if (!Loader.isvalid() && Cat.AllowDefaultLoader) Loader = DefaultLoader;
				if (!Loader.isvalid()) continue;

				//???set category & table once outside a loop?
				for (int j = 0; j < Cat.InstDataset->GetValueTable()->GetRowCount(); j++)
					Loader->Load(Categories.KeyAtIndex(i), Cat.InstDataset->GetValueTable(), j);
			}
		}
	}
}
//---------------------------------------------------------------------

void CEntityFactory::DeleteEntityInstance(CEntity* pEntity)
{
	n_assert(pEntity);
	if(pEntity->GetAttrTable()->IsRowValid(pEntity->GetAttrTableRowIndex()))
		pEntity->GetAttrTable()->DeleteRow(pEntity->GetAttrTableRowIndex());
	//Categories[pEntity->GetCategory()].InstDataset->CommitDeletedRows(); //???need or on commit?
}
//---------------------------------------------------------------------

void CEntityFactory::RenameEntityInstance(CEntity* pEntity, CStrID NewID) const
{
	n_assert(pEntity);

	CStrID OldID = pEntity->GetUniqueID();

	nString SQL;
	SQL.Format("UPDATE %s SET GUID='%s' WHERE GUID='%s'",
		Categories[pEntity->Category].InstTableName.Get(), NewID.CStr(), OldID.CStr());
	PCommand Cmd = CCommand::Create();
	n_assert(Cmd->Execute(LoaderSrv->GetGameDB(), SQL));

	PProperty Prop;
	for (int i = 0; i < PropStorage.Size(); i++)
		if (PropStorage[i]->Get(OldID, Prop))
		{
			PropStorage[i]->Erase(OldID);
			PropStorage[i]->Add(NewID, Prop);;
		}
}
//---------------------------------------------------------------------

void CEntityFactory::CommitChangesToDB()
{
	for (int i = 0; i < Categories.Size(); i++)
	{
		CEntityCat& Cat = Categories.ValueAtIndex(i);
#ifdef _EDITOR
		if (Cat.TplDataset.isvalid()) Cat.TplDataset->CommitChanges();
#endif
		if (Cat.InstDataset.isvalid()) Cat.InstDataset->CommitChanges();
	}
}
//---------------------------------------------------------------------

bool CEntityFactory::OnStaticDBClose(const Events::CEventBase& Event)
{
	//???don't kill, just clear cmds?
	for (int i = 0; i < Categories.Size(); i++)
		Categories.ValueAtIndex(i).TplDataset = NULL;
	OK;
}
//---------------------------------------------------------------------

bool CEntityFactory::OnGameDBClose(const Events::CEventBase& Event)
{
	//???don't kill, just clear cmds?
	for (int i = 0; i < Categories.Size(); i++)
		Categories.ValueAtIndex(i).InstDataset = NULL;
	OK;
}
//---------------------------------------------------------------------

}