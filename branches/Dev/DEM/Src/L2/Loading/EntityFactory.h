#pragma once
#ifndef __DEM_L2_ENTITY_FACTORY_H__
#define __DEM_L2_ENTITY_FACTORY_H__

#include <Game/Manager.h>
#include <Game/Property.h>
#include <Loading/EntityLoaderBase.h>
#include <DB/Dataset.h>
#include <DB/Database.h>
#include <util/nhashmap2.h>

// The CEntityFactory is responsible for creating new game entities.
// CEntityFactory must be extended by Mangalore applications if the
// application needs new game entity classes.
// The CEntityFactory loads the file data:tables/EntityCats.hrd
// on creation, which contains the construction blueprints for the
// entity types of your application. This file defines entity types
// by the properties which are added to them.
// Based on mangalore EntityFactory_(C) 2005 Radon Labs GmbH

namespace Attr
{
	DeclareAttr(GUID);
}

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Game
{
	typedef Ptr<class CEntity> PEntity;
}

namespace Loading
{
using namespace Game;

#define EntityFct Loading::CEntityFactory::Instance()

class CEntityFactory
{
private:

	bool _IsActive;

	struct CPropertyInfo
	{
		CPropertyStorage*	pStorage;
		int					ActivePools;
		CPropertyInfo(): pStorage(NULL), ActivePools(0) {}
		CPropertyInfo(CPropertyStorage& Storage, int Pools): pStorage(&Storage), ActivePools(Pools) {}
	};

	struct CEntityCat
	{
		nString					CppClass;
		nArray<nString>			Properties;
		nArray<DB::CAttrID>		Attrs;
		nString					TplTableName;
		nString					InstTableName;
		DB::PDataset			TplDataset;
		DB::PDataset			InstDataset;
		Ptr<CEntityLoaderBase>	Loader;
		bool					AllowDefaultLoader;
	};

	Ptr<CEntityLoaderBase>			DefaultLoader;
	nHashMap2<CPropertyInfo>		PropertyMeta; //???index by type or by RTTI?
	nArray<CPropertyStorage*>		PropStorage; //!!!tmp for iteration through all!
	nDictionary<CStrID, CEntityCat>	Categories;

	CProperty*	AttachProperty(CEntity& Entity, CPropertyStorage* pStorage) const;
	void		CreateEntityCat(CStrID Name, const Data::PParams& Desc);
	int			FindTemplate(CStrID UID, const CEntityCat& Cat);

public:

	CEntityFactory(): PropertyMeta(CPropertyInfo(), 64) {}

	static CEntityFactory* Instance() { static CEntityFactory Singleton; return &Singleton; }

	void				Init();
	void				Release();
	bool				IsActive() const { return _IsActive; }

	CEntity*			CreateEntityByEntity(CStrID GUID, CEntity* pTplEntity, EntityPool Pool = LivePool) const;
	
	// Create new entity
	//!!!tmp not const!
	CEntity*			CreateEntityByClassName(const nString& CppClassName) const;
	CEntity*			CreateEntityByCategory(CStrID GUID, CStrID Category, EntityPool = LivePool);// const;
	CEntity*			CreateEntityByTemplate(CStrID GUID, CStrID Category, CStrID TplName, EntityPool = LivePool);// const;
	CEntity*			CreateTmpEntity(CStrID GUID, CStrID Category, DB::PValueTable Table, int Row) const;

	// Load entity from DB table
	PEntity				CreateEntityByCategory(CStrID Category, DB::CValueTable* pTable, int RowIdx) const;

	CProperty*			AttachProperty(Game::CEntity& Entity, const nString& TypeName) const;
	template<class T>T*	AttachProperty(Game::CEntity& Entity) const;
	void				DetachProperty(Game::CEntity& Entity, const nString& TypeName) const;
	void				DetachAllProperties(Game::CEntity& Entity) const;

	bool				RegisterPropertyMeta(const Core::CRTTI& RTTI, CPropertyStorage& Storage, int ActivePools);

	void				SetLoader(CStrID Category, CEntityLoaderBase* pLoader);
	void				SetDefaultLoader(CEntityLoaderBase* pLoader) { DefaultLoader = pLoader; }
	void				RemoveLoader(CStrID Category);
	void				RemoveDefaultLoader() { DefaultLoader = NULL; }
	void				RemoveAllLoaders();

	void				LoadEntityTemplates();
	void				UnloadEntityTemplates();
	DB::CDataset*		GetTemplate(CStrID UID, CStrID Category, bool CreateIfNotExist);
	void				DeleteTemplate(CStrID UID, CStrID Category);

	void				LoadEntityInstances(const nString& LevelName);
	void				UnloadEntityInstances(); //???specify level?
	void				InitEmptyInstanceDatasets() { LoadEntityInstances(NULL); }
	void				DeleteEntityInstance(CEntity* pEntity);
	void				RenameEntityInstance(CEntity* pEntity, CStrID NewID) const;
	void				CommitChangesToDB();

	// No LevelID arg since we guarantee uniqueness of entity ID in a whole DB
	template<class T>
	bool				GetEntityAttribute(const nString& UID, DB::CAttrID AttrID, T& Out, const char* Category = NULL);

	bool				HasTemplateTable(const nString& CategoryName) const;
	//const DB::PValueTable& GetTemplateTable(const Util::String& categoryName) const;
	bool				HasInstanceTable(const nString& CategoryName) const;
	//const DB::PValueTable& GetInstanceTable(const Util::String& categoryName) const;

//!!!
//#ifdef _EDITOR
	int					GetCategoryCount() const { return Categories.Size(); }
	const char*			GetCategoryName(int Idx) const { return Categories.KeyAtIndex(Idx).CStr(); }
	const CEntityCat&	GetCategory(int Idx) const { return Categories.ValueAtIndex(Idx); }
	int					GetTemplateCount(CStrID CatID) const { return (Categories[CatID].TplDataset.isvalid()) ? Categories[CatID].TplDataset->GetRowCount() : 0; }
	const char*			GetTemplateID(CStrID CatID, int Idx) const;
	void				CreateNewEntityCat(CStrID Name, const Data::PParams& Desc, bool CreateInstDataset);
//#endif
};
//---------------------------------------------------------------------

inline bool CEntityFactory::RegisterPropertyMeta(const Core::CRTTI& RTTI,
												 CPropertyStorage& Storage,
												 int ActivePools)
{
	n_assert2(!PropertyMeta.Contains(RTTI.GetName().Get()), "Registering already registered property meta");
	PropertyMeta.Add(RTTI.GetName().Get(), CPropertyInfo(Storage, ActivePools));
	//???tmp?
	PropStorage.Append(&Storage);
	OK;
}
//---------------------------------------------------------------------

template<class T> T* CEntityFactory::AttachProperty(Game::CEntity& Entity) const
{
	if (T::Pools & Entity.GetPool())
	{
		PProperty Prop;
		if (!T::Storage.Get(Entity.GetUniqueID(), Prop))
		{
			Prop = T::Create();
			n_assert(Prop.isvalid());
			T::Storage.Add(Entity.GetUniqueID(), Prop);
			Prop->SetEntity(&Entity);
		}
		return (T*)Prop.get_unsafe();
	}

	return NULL;
}
//---------------------------------------------------------------------

inline void CEntityFactory::SetLoader(CStrID Category, CEntityLoaderBase* pLoader)
{
	CEntityCat& Cat = Categories[Category];
	Cat.Loader = pLoader;
	if (!pLoader) Cat.AllowDefaultLoader = false;
}
//---------------------------------------------------------------------

inline void CEntityFactory::RemoveLoader(CStrID Category)
{
	CEntityCat& Cat = Categories[Category];
	Cat.Loader = NULL;
	Cat.AllowDefaultLoader = true;
}
//---------------------------------------------------------------------

inline void CEntityFactory::RemoveAllLoaders()
{
	DefaultLoader = NULL;
	for (int i = 0; i < Categories.Size(); i++)
	{
		Categories.ValueAtIndex(i).Loader = NULL;
		Categories.ValueAtIndex(i).AllowDefaultLoader = true;
	}
}
//---------------------------------------------------------------------

// Send NULL Category to search in all categories
// NULL will be returned if no entity/attribute found
template<class T> bool CEntityFactory::GetEntityAttribute(const nString& UID, DB::CAttrID AttrID, T& Out, const char* Category)
{
	nString Where = "GUID='" + UID + "'";

	if (Category)
	{
		Ptr<DB::CDataset> DS = Categories[CStrID(Category)].InstDataset->GetTable()->CreateDataset();
		DS->AddColumn(AttrID);
		DS->SetWhereClause(Where);
		DS->PerformQuery();
		if (DS->GetValueTable()->GetRowCount() > 0)
		{
			n_assert2(DS->GetValueTable()->GetRowCount() == 1, "Duplicate entity ID found!");
			Out = DS->GetValueTable()->Get<T>(0, 0);
			OK;
		}
	}
	else
		for (int i = 0; i < Categories.Size(); i++)
		{
			DB::PDataset InstDS = Categories.ValueAtIndex(i).InstDataset;
			if (!InstDS.isvalid()) continue;

			DB::PDataset DS = InstDS->GetTable()->CreateDataset();
			DS->AddColumn(AttrID);
			DS->SetWhereClause(Where);
			DS->PerformQuery();
			if (DS->GetValueTable()->GetRowCount() > 0)
			{
				n_assert2(DS->GetValueTable()->GetRowCount() == 1, "Duplicate entity ID found!");
				Out = DS->GetValueTable()->Get<T>(0, 0);
				OK;
			}
		}
	FAIL;
}
//---------------------------------------------------------------------

}

#endif
