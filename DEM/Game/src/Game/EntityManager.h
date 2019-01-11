#pragma once
#ifndef __DEM_L2_ENTITY_MANAGER_H__
#define __DEM_L2_ENTITY_MANAGER_H__

#include <Game/Property.h>
#include <Events/EventsFwd.h>
#include <Data/Dictionary.h>

// The entity manager creates and manages entities and allows to
// register properties to be used by entities.

namespace Data
{
	class CData;
}

namespace Game
{
typedef Ptr<class CEntity> PEntity;
class CGameLevel;

class CEntityManager
{
protected:

	CDict<const Core::CRTTI*, CPropertyStorage**>	PropStorages;
	CArray<PEntity>									Entities;
	CHashTable<CStrID, CEntity*>					UIDToEntity;
	CArray<CStrID>									EntitiesToDelete;

	void		DeleteEntity(IPTR Idx);

public:

	CEntityManager();
	~CEntityManager();

	PEntity		CreateEntity(CStrID UID, CGameLevel& Level);
	bool		RenameEntity(CEntity& Entity, CStrID NewUID);
	PEntity		CloneEntity(const CEntity& Entity, CStrID UID);
	void		DeleteEntity(CEntity& Entity);
	void		DeleteEntity(CStrID UID);
	void		DeleteEntities(const CGameLevel& Level);
	void		DeleteAllEntities();
	void		RequestDestruction(CStrID UID) { EntitiesToDelete.Add(UID); } //???deactivate? test with item picking!
	void		DeferredDeleteEntities();

	UPTR		GetEntityCount() const { return Entities.GetCount(); }
	CEntity*	GetEntity(IPTR Idx) const { return Entities[Idx].Get(); }
	CEntity*	GetEntity(CStrID UID) const;
	bool		EntityExists(CStrID UID) const { return !!GetEntity(UID); }
	void		GetEntitiesByLevel(const CGameLevel* pLevel, CArray<CEntity*>& Out) const;

	template<class T>
	bool		RegisterProperty(UPTR TableCapacity = 32);
	template<class T>
	bool		UnregisterProperty();
	CProperty*	AttachProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const;
	template<class T>
	T*			AttachProperty(CEntity& Entity) const;
	void		RemoveProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const;
	template<class T>
	void		RemoveProperty(CEntity& Entity) const;
	CProperty*	GetProperty(CStrID EntityID, const Core::CRTTI* pRTTI) const;
	template<class T>
	T*			GetProperty(CStrID EntityID) const;
	void		GetPropertiesOfEntity(CStrID EntityID, CArray<CProperty*>& Out) const;
};

inline void CEntityManager::DeleteEntity(CStrID UID)
{
	CEntity* pEnt = GetEntity(UID);
	if (pEnt) DeleteEntity(*pEnt);
}
//---------------------------------------------------------------------

inline void CEntityManager::DeferredDeleteEntities()
{
	for (UPTR i = 0; i < EntitiesToDelete.GetCount(); ++i)
		DeleteEntity(EntitiesToDelete[i]);
	EntitiesToDelete.Clear();
}
//---------------------------------------------------------------------

template<class T>
bool CEntityManager::RegisterProperty(UPTR TableCapacity)
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
			It->Get()->Deactivate();
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
	return (T*)Prop.Get();
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

template<class T>
T* CEntityManager::GetProperty(CStrID EntityID) const
{
	PProperty Prop;
	if (T::pStorage && T::pStorage->Get(EntityID, Prop))
		if (!Prop->IsA(T::RTTI)) return NULL;
	return (T*)Prop.Get();
}
//---------------------------------------------------------------------

}

#endif
