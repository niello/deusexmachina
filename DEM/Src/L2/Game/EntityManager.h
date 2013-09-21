#pragma once
#ifndef __DEM_L2_ENTITY_MANAGER_H__
#define __DEM_L2_ENTITY_MANAGER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <Game/Entity.h>
#include <Game/Property.h>
#include <Events/EventsFwd.h>
#include <Data/Dictionary.h>

// The entity manager creates and manages entities and allows to
// register properties to be usable by entities.

namespace Game
{
#define EntityMgr Game::CEntityManager::Instance()

class CEntityManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CEntityManager);

protected:

	CDict<const Core::CRTTI*, CPropertyStorage**>	PropStorages;
	CArray<PEntity>									Entities;
	CHashTable<CStrID, CEntity*>					UIDToEntity;
	CDict<CStrID, CStrID>							Aliases;
	CArray<CStrID>									EntitiesToDelete;

	void		DeleteEntity(int Idx);

public:

	CEntityManager(): Entities(256, 256), UIDToEntity(512) { __ConstructSingleton; Entities.Flags.Clear(Array_KeepOrder); }
	~CEntityManager();

	PEntity		CreateEntity(CStrID UID, CGameLevel& Level);
	bool		RenameEntity(CEntity& Entity, CStrID NewUID);
	PEntity		CloneEntity(const CEntity& Entity, CStrID UID);
	void		DeleteEntity(CEntity& Entity);
	void		DeleteEntity(CStrID UID);
	void		DeleteEntities(const CGameLevel& Level);
	void		DeleteAllEntities() { while (Entities.GetCount() > 0) DeleteEntity(*Entities.Back()); }
	void		RequestDestruction(CStrID UID) { EntitiesToDelete.Add(UID); }
	void		DeferredDeleteEntities();

	int			GetEntityCount() const { return Entities.GetCount(); }
	CEntity*	GetEntity(int Idx) const { return Entities[Idx].GetUnsafe(); }
	CEntity*	GetEntity(CStrID UID, bool SearchInAliases = false) const;
	bool		EntityExists(CStrID UID, bool SearchInAliases = false) const { return !!GetEntity(UID, SearchInAliases); }

	CEntity*	FindEntityByAttr(CStrID AttrID, const Data::CData& Value) const; //???find first - find next?
	void		FindEntitiesByAttr(CStrID AttrID, const Data::CData& Value, CArray<CEntity*>& Out) const;
	void		GetEntitiesByLevel(const CGameLevel* pLevel, CArray<CEntity*>& Out) const;

	//???find first/all by property? - in fact iterates through the storage / CopyToArray

	bool		SetEntityAlias(CStrID Alias, CStrID UID) { if (!UID.IsValid()) FAIL; Aliases.Set(Alias, UID); OK; }
	void		RemoveEntityAlias(CStrID Alias) { Aliases.Remove(Alias); }

	template<class T>
	bool		RegisterProperty(DWORD TableCapacity = 32);
	template<class T>
	bool		UnregisterProperty();
	CProperty*	AttachProperty(CEntity& Entity, const CString& ClassName) const { return AttachProperty(Entity, Factory->GetRTTI(ClassName)); }
	CProperty*	AttachProperty(CEntity& Entity, Data::CFourCC ClassFourCC) const { return AttachProperty(Entity, Factory->GetRTTI(ClassFourCC)); }
	CProperty*	AttachProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const;
	template<class T>
	T*			AttachProperty(CEntity& Entity) const;
	void		RemoveProperty(CEntity& Entity, const CString& ClassName) const { return RemoveProperty(Entity, Factory->GetRTTI(ClassName)); }
	void		RemoveProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const;
	template<class T>
	void		RemoveProperty(CEntity& Entity) const;
	void		GetPropertiesOfEntity(CStrID EntityID, CArray<CProperty*>& Out) const;
};

typedef Ptr<CEntityManager> PEntityManager;

inline void CEntityManager::DeleteEntity(CEntity& Entity)
{
	int Idx = Entities.FindIndex(&Entity);
	if (Idx != INVALID_INDEX) DeleteEntity(Idx);
}
//---------------------------------------------------------------------

inline void CEntityManager::DeleteEntity(CStrID UID)
{
	CEntity* pEnt = GetEntity(UID, true);
	if (pEnt) DeleteEntity(*pEnt);
}
//---------------------------------------------------------------------

inline void CEntityManager::DeferredDeleteEntities()
{
	for (int i = 0; i < EntitiesToDelete.GetCount(); ++i)
		DeleteEntity(EntitiesToDelete[i]);
	EntitiesToDelete.Clear();
}
//---------------------------------------------------------------------

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
		for (CPropertyStorage::CIterator It = T::pStorage->Begin(); It; ++It)
			It->GetUnsafe()->Deactivate();
		T::pStorage->Clear();
	}

	PropStorages.Remove(&T::RTTI);
	n_delete(T::pStorage);
	T::pStorage = NULL;
	OK;
}
//---------------------------------------------------------------------

template<class T>
T* CEntityManager::AttachProperty(Game::CEntity& Entity) const
{
	n_assert_dbg(T::RTTI.IsDerivedFrom(CProperty::RTTI));
	n_assert2_dbg(T::pStorage, (CString("Property ") + T::RTTI.GetName() + " is not registered!").CStr());
	if (!T::pStorage) return NULL;

	PProperty Prop;
	if (!T::pStorage->Get(Entity.GetUID(), Prop))
	{
		Prop = T::CreateInstance();
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
	n_assert2_dbg(T::pStorage, (CString("Property ") + T::RTTI.GetName() + " is not registered!").CStr());
	if (!T::pStorage) return;

	PProperty Prop;
	if (T::pStorage->Get(Entity.GetUID(), Prop))
	{
		if (Prop->IsActive()) Prop->Deactivate();

		//!!!remove by iterator!
		T::pStorage->Remove(Entity.GetUID());
	}
}
//---------------------------------------------------------------------

}

#endif
