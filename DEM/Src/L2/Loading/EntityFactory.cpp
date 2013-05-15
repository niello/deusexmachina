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

void CEntityFactory::Init()
{
	n_assert(!_IsActive);
	_IsActive = true;
	
	Categories.Clear();

	Data::PParams P = DataSrv->LoadHRD("data:tables/EntityCats.hrd");
	if (!P.IsValid()) n_error("Loading::CEntityFactory: Error loading EntityCats.hrd!");

	for (int i = 0; i < P->GetCount(); i++)
		CreateEntityCat(P->Get(i).GetName(), P->Get(i).GetValue<Data::PParams>());
}
//---------------------------------------------------------------------

void CEntityFactory::Release()
{
	n_assert(_IsActive);
	RemoveAllLoaders();
	Categories.Clear();
	_IsActive = false;
}
//---------------------------------------------------------------------

//Create an entity from its category name. The category name is looked
//up in the EntityCats.hrd file to check what properties must be attached
//to the entity. All required properties will be attached, and all
//attributes will be initialised in their default state.
//!!!!!tmp not const!
CEntity* CEntityFactory::CreateEntityByCategory(CStrID GUID, CStrID Category) //const
{
	int Idx = Categories.FindIndex(Category);
	if (Idx != INVALID_INDEX)
	{
		const CEntityCat& Cat = Categories.ValueAtIndex(Idx);
		n_assert(Cat.InstDataset.IsValid());
		
		CEntity* pEntity = n_new(CEntity);
		pEntity->SetAttrTable(Cat.InstDataset->GetValueTable());
		pEntity->SetAttrTableRowIndex(Cat.InstDataset->GetValueTable()->AddRow());
		pEntity->SetCategory(Category);
		pEntity->SetUID(GUID);

		for (int i = 0; i < Cat.Properties.GetCount(); i++)
			AttachProperty(*pEntity, StrPropPrefix + Cat.Properties[i]);

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

CEntity* CEntityFactory::CreateEntityByTemplate(CStrID GUID, CStrID Category, CStrID TplName)
{
	n_assert(Category.IsValid());
	n_assert(TplName.IsValid());

	const CEntityCat& Cat = Categories[Category];
	
	int TplIdx = FindTemplate(TplName, Cat);
	if (TplIdx == INVALID_INDEX) return NULL;

	CEntity* pEntity = n_new(CEntity);
	pEntity->SetAttrTable(Cat.InstDataset->GetValueTable());
	pEntity->SetAttrTableRowIndex(Cat.InstDataset->GetValueTable()->CopyExtRow(Cat.TplDataset->GetValueTable(), TplIdx, true));
	pEntity->SetCategory(Category);
	pEntity->SetUID(GUID);

	for (int i = 0; i < Cat.Properties.GetCount(); i++)
		AttachProperty(*pEntity, StrPropPrefix + Cat.Properties[i]);

	return pEntity;
}
//---------------------------------------------------------------------

// Create a entity as a clone of a existing one. A new GUID will be assigned.
CEntity* CEntityFactory::CreateEntityByEntity(CStrID GUID, CEntity* pTplEntity) const
{
	n_assert(pTplEntity);
	n_assert(pTplEntity->GetCategory().IsValid());
	n_assert(pTplEntity->HasAttr(Attr::GUID) && pTplEntity->Get<CStrID>(Attr::GUID).IsValid());

	//???!!!tpl name arg as CStrID, not nString?!
	CEntity* pEntity = NULL;// CreateEntityByTemplate(GUID, pTplEntity->GetCategory().CStr(),//???CStrID arg?
		//pTplEntity->Get<CStrID>(Attr::GUID).CStr());
	n_assert(pEntity);

	//const nArray<DB::CAttr>& TplAttrs = pTplEntity->GetAttrs();
	//for (int i = 0; i < TplAttrs.GetCount(); i++)
	//	if (TplAttrs[i].GetAttrID() != Attr::GUID)
	//		pEntity->SetAttr(TplAttrs[i]);

	return pEntity;
}
//---------------------------------------------------------------------

CEntity* CEntityFactory::CreateTmpEntity(CStrID GUID, CStrID Category, PValueTable Table, int Row) const
{
	CEntity* pEntity = n_new(CEntity);
	pEntity->SetCategory(Category); //???add to dummy table all attrs if category exists?
	pEntity->SetAttrTable(Table);
	pEntity->SetAttrTableRowIndex(Row);
	pEntity->SetUID(GUID);
	return pEntity;
}
//---------------------------------------------------------------------

PEntity CEntityFactory::CreateEntityByCategory(CStrID Category, DB::CValueTable* pTable, int RowIdx) const
{
	int Idx = Categories.FindIndex(Category);
	if (Idx != INVALID_INDEX)
	{
		const CEntityCat& Cat = Categories.ValueAtIndex(Idx);
		PEntity pEntity = n_new(CEntity);

		pEntity->SetAttrTable(pTable);
		pEntity->SetAttrTableRowIndex(RowIdx);
		pEntity->SetUniqueIDFromAttrTable();
		pEntity->SetCategory(Category);

		for (int i = 0; i < Cat.Properties.GetCount(); i++)
			AttachProperty(*pEntity, StrPropPrefix + Cat.Properties[i]);
		
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
	if (!PropertyMeta.Get(TypeName.CStr(), PropInfo))
		n_error("No such property \"%s\"", TypeName.CStr());

	PProperty Prop;
	if (!PropInfo.pStorage->Get(Entity.GetUID(), Prop))
	{
		Prop = (CProperty*)Factory->Create(TypeName);
		PropInfo.pStorage->Add(Entity.GetUID(), Prop);
		Prop->SetEntity(&Entity);
	}
	return Prop.GetUnsafe();

	return NULL;
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
	for (int i = 0; i < Props.GetCount(); ++i)
	{
		const nString& PropName = Props[i].GetValue<nString>();
		New.Properties.Append(PropName);
		PProperty Prop = (CProperty*)Factory->Create(StrPropPrefix + PropName);
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
	for (int i = 0; i < Categories.GetCount(); i++)
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

void CEntityFactory::UnloadEntityTemplates()
{
	for (int i = 0; i < Categories.GetCount(); i++)
		Categories.ValueAtIndex(i).TplDataset = NULL;
}
//---------------------------------------------------------------------

DB::CDataset* CEntityFactory::GetTemplate(CStrID UID, CStrID Category, bool CreateIfNotExist)
{
	if (!Categories.Contains(Category)) return NULL;

	CEntityCat& Cat = Categories[Category];

	if (!Cat.TplDataset.IsValid()) return NULL;

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

	if (Cat.TplDataset.IsValid())
		for (int i = 0; i < Cat.TplDataset->GetRowCount(); i++)
			if (Cat.TplDataset->GetValueTable()->IsRowValid(i))
				if (Cat.TplDataset->GetValueTable()->Get<CStrID>(Attr::GUID, i) == UID)
					Cat.TplDataset->GetValueTable()->DeleteRow(i);
}
//---------------------------------------------------------------------

int CEntityFactory::FindTemplate(CStrID UID, const CEntityCat& Cat)
{
	n_assert(Cat.TplDataset.IsValid());

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

	for (int i = 0; i < Categories.GetCount(); i++)
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
				if (!Loader.IsValid() && Cat.AllowDefaultLoader) Loader = DefaultLoader;
				if (!Loader.IsValid()) continue;

				//???set category & table once outside a loop?
				for (int j = 0; j < Cat.InstDataset->GetValueTable()->GetRowCount(); j++)
					Loader->Load(Categories.KeyAtIndex(i), Cat.InstDataset->GetValueTable(), j);
			}
		}
	}
}
//---------------------------------------------------------------------

void CEntityFactory::UnloadEntityInstances()
{
	for (int i = 0; i < Categories.GetCount(); i++)
		Categories.ValueAtIndex(i).InstDataset = NULL;
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

	CStrID OldID = pEntity->GetUID();

	nString SQL;
	SQL.Format("UPDATE %s SET GUID='%s' WHERE GUID='%s'",
		Categories[pEntity->Category].InstTableName.CStr(), NewID.CStr(), OldID.CStr());
	PCommand Cmd = CCommand::Create();
	n_assert(Cmd->Execute(LoaderSrv->GetGameDB(), SQL));

	PProperty Prop;
	for (int i = 0; i < PropStorage.GetCount(); i++)
		if (PropStorage[i]->Get(OldID, Prop))
		{
			PropStorage[i]->Erase(OldID);
			PropStorage[i]->Add(NewID, Prop);;
		}
}
//---------------------------------------------------------------------

void CEntityFactory::CommitChangesToDB()
{
	for (int i = 0; i < Categories.GetCount(); i++)
	{
		CEntityCat& Cat = Categories.ValueAtIndex(i);
#ifdef _EDITOR
		if (Cat.TplDataset.IsValid()) Cat.TplDataset->CommitChanges();
#endif
		if (Cat.InstDataset.IsValid()) Cat.InstDataset->CommitChanges();
	}
}
//---------------------------------------------------------------------

}