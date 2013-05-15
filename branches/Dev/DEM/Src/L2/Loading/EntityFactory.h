#pragma once
#ifndef __DEM_L2_ENTITY_FACTORY_H__
#define __DEM_L2_ENTITY_FACTORY_H__

#include <Game/Property.h>
#include <DB/Dataset.h>
#include <DB/Database.h>
#include <util/HashMap.h>

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
		CPropertyInfo(): pStorage(NULL) {}
		CPropertyInfo(CPropertyStorage& Storage): pStorage(&Storage) {}
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
		bool					AllowDefaultLoader;
	};

	CHashMap<CPropertyInfo>			PropertyMeta; //???index by type or by RTTI?
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

	CEntity*			CreateEntityByEntity(CStrID GUID, CEntity* pTplEntity) const;
	
	// Create new entity
	//!!!tmp not const!
	CEntity*			CreateEntityByCategory(CStrID GUID, CStrID Category);// const;
	CEntity*			CreateEntityByTemplate(CStrID GUID, CStrID Category, CStrID TplName);// const;
	CEntity*			CreateTmpEntity(CStrID GUID, CStrID Category, DB::PValueTable Table, int Row) const;

	// Load entity from DB table
	PEntity				CreateEntityByCategory(CStrID Category, DB::CValueTable* pTable, int RowIdx) const;
};
//---------------------------------------------------------------------

}

#endif
