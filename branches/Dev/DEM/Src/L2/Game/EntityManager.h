#pragma once
#ifndef __DEM_L2_ENTITY_MANAGER_H__
#define __DEM_L2_ENTITY_MANAGER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <Game/Entity.h>
#include <Game/Property.h>
#include <Events/Events.h>
#include <util/ndictionary.h>

// The entity manager creates and manages entities and allows to
// register properties to be usable by entities.

//!!!how to manage entities on different levels?

namespace Game
{
using namespace Core;

#define EntityMgr Game::CEntityManager::Instance()

class CEntityManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CEntityManager);

protected:

	nDictionary<Core::CRTTI*, CPropertyStorage**>	PropStorages;
	nArray<PEntity>									Entities;	//???need? CHashTable has iterator
	CHashTable<CStrID, CEntity*>					UIDToEntity;
	nDictionary<CStrID, CStrID>						Aliases;

public:

	CEntityManager(): Entities(256, 256), UIDToEntity(512) { __ConstructSingleton; }
	~CEntityManager() { n_assert(!Entities.GetCount() && !UIDToEntity.GetCount()); __DestructSingleton; }

	void		Open();
	void		Close();

	PEntity		CreateEntity(CStrID UID, CStrID LevelID);
	bool		RenameEntity(CEntity& Entity, CStrID NewUID);
	PEntity		CloneEntity(const CEntity& Entity, CStrID UID);
	void		DeleteEntity(CEntity& Entity);
	void		DeleteEntity(CStrID UID) { CEntity* pEnt = GetEntity(UID, true); if (pEnt) DeleteEntity(*pEnt); }
	void		DeleteAllEntities() { while (Entities.GetCount() > 0) DeleteEntity(*Entities.Back()); }

	template<class T>
	bool		RegisterProperty(DWORD TableCapacity = 32);
	template<class T>
	bool		UnregisterProperty();
	CProperty*	AttachProperty(CEntity& Entity, Core::CRTTI& Type) const;
	//!!!CProperty*	AttachProperty(CEntity& Entity, const nString& TypeName) const; //???or GetRTTI(TypeName)?
	template<class T>
	T*			AttachProperty(CEntity& Entity) const;
	void		RemoveProperty(CEntity& Entity, Core::CRTTI& Type) const;
	template<class T>
	void		RemoveProperty(CEntity& Entity) const;

	bool		SetEntityAlias(CStrID Alias, CStrID UID) { if (!UID.IsValid()) FAIL; Aliases.Set(Alias, UID); OK; }
	void		RemoveEntityAlias(CStrID Alias) { Aliases.Erase(Alias); }

	int			GetEntityCount() const { return Entities.GetCount(); }
	CEntity*	GetEntity(int Idx) const { return Entities[Idx].GetUnsafe(); }
	CEntity*	GetEntity(CStrID UID, bool SearchInAliases = false) const;
	bool		EntityExists(CStrID UID, bool SearchInAliases = false) const { return !!GetEntity(UID, SearchInAliases); }

	CEntity*	FindEntityByAttr(CStrID AttrID, const Data::CData& Value) const; //???find first - find next?
	void		FindEntitiesByAttr(CStrID AttrID, const Data::CData& Value, nArray<CEntity*>& Out) const;

	//???find first/all by property? - in fact iterates through the storage / CopyToArray
};

typedef Ptr<CEntityManager> PEntityManager;

template<class T>
bool CEntityManager::RegisterProperty(DWORD TableCapacity)
{
	n_assert_dbg(T::RTTI.IsDerivedFrom(CProperty::RTTI));
	if (T::pStorage) FAIL;
	T::pStorage = n_new(CPropertyStorage(TableCapacity));
	PropStorages.Add(&T::RTTI, &T::pStorage);
	OK;
}
//---------------------------------------------------------------------

template<class T>
bool CEntityManager::UnregisterProperty()
{
	n_assert_dbg(T::RTTI.IsDerivedFrom(CProperty::RTTI));
	if (!T::pStorage) FAIL;

	if (T::pStorage->GetCount())
	{
		CPropertyStorage::CIterator It = T::pStorage->Begin();
		while (!It.IsEnd())
		{
			n_error("CEntityManager::UnregisterProperty() -> WRITE Deactivate and detach!");
			//!!!Deactivate & detach property!
			++It;
		}
	}

	PropStorages.Erase(&T::RTTI);
	n_delete(T::pStorage);
	T::pStorage = NULL;
	OK;
}
//---------------------------------------------------------------------

template<class T>
T* CEntityManager::AttachProperty(Game::CEntity& Entity) const
{
	n_assert_dbg(T::RTTI.IsDerivedFrom(CProperty::RTTI));
	n_assert2_dbg(T::pStorage, (nString("Property ") + T::RTTI.GetName() + " is not registered!").CStr());
	if (!T::pStorage) return NULL;

	PProperty Prop;
	if (!T::pStorage->Get(Entity.GetUID(), Prop))
	{
		Prop = T::Create();
		n_assert(Prop.IsValid());
		T::pStorage->Add(Entity.GetUID(), Prop);
		Prop->SetEntity(&Entity);
	}
	return (T*)Prop.GetUnsafe();
}
//---------------------------------------------------------------------

template<class T>
void CEntityManager::RemoveProperty(Game::CEntity& Entity) const
{
	n_assert_dbg(T::RTTI.IsDerivedFrom(CProperty::RTTI));
	n_assert2_dbg(T::pStorage, (nString("Property ") + T::RTTI.GetName() + " is not registered!").CStr());
	if (!T::pStorage) return;
	T::pStorage->Erase(Entity.GetUID());
}
//---------------------------------------------------------------------

}

#endif
