#include "EntityManager.h"

#include <Game/Entity.h>
#include <Game/GameLevel.h>
#include <Events/EventServer.h>

namespace Game
{

CEntityManager::CEntityManager(): Entities(256, 256), UIDToEntity(512)
{
	Entities.Flags.Clear(Array_KeepOrder);
}
//---------------------------------------------------------------------

CEntityManager::~CEntityManager()
{
	DeleteAllEntities();

	// Delete storages allocated in RegisterProperty and clear storage refs in prop's static fields.
	// We don't need to deactivate and detach properties, DeleteAllEntities deleted all prop instances.
	for (UPTR i = 0; i < PropStorages.GetCount(); ++i)
	{
		n_assert_dbg(!(*PropStorages.ValueAt(i))->GetCount());
		n_delete(*PropStorages.ValueAt(i));
		*PropStorages.ValueAt(i) = nullptr;
	}
	PropStorages.Clear();
}
//---------------------------------------------------------------------

PEntity CEntityManager::CreateEntity(CStrID UID, CGameLevel& Level)
{
	n_assert(UID.IsValid());
	n_assert(!EntityExists(UID)); //???return nullptr or existing entity?
	PEntity Entity = n_new(CEntity(UID));
	Entity->SetLevel(&Level);
	Entities.Add(Entity);
	UIDToEntity.Add(Entity->GetUID(), Entity.Get());
	return Entity;
}
//---------------------------------------------------------------------

bool CEntityManager::RenameEntity(CEntity& Entity, CStrID NewUID)
{
	if (!NewUID.IsValid()) FAIL;
	if (Entity.GetUID() == NewUID) OK;
	if (EntityExists(NewUID)) FAIL;
	UIDToEntity.Remove(Entity.GetUID());
	Entity.SetUID(NewUID);
	UIDToEntity.Add(Entity.GetUID(), &Entity);
	Entity.FireEvent(CStrID("OnEntityRenamed"));
	OK;
}
//---------------------------------------------------------------------

PEntity CEntityManager::CloneEntity(const CEntity& Entity, CStrID UID)
{
	// create entity with new UID
	// copy attributes
	// attach all the same properties
	// if props have copy constructor or Clone method, exploit it
	Sys::Error("CEntityManager::CloneEntity() -> IMPLEMENT ME!!!");
	return nullptr;
}
//---------------------------------------------------------------------

void CEntityManager::DeleteEntity(CEntity& Entity)
{
	IPTR Idx = Entities.FindIndex(&Entity);
	if (Idx != INVALID_INDEX) DeleteEntity(Idx);
}
//---------------------------------------------------------------------

void CEntityManager::DeleteEntity(IPTR Idx)
{
	CEntity& Entity = *Entities[Idx];

	n_assert_dbg(!Entity.IsActivating() && !Entity.IsDeactivating());

	if (Entity.IsActive()) Entity.Deactivate();

	// Remove all properties of this entity
	for (UPTR i = 0; i < PropStorages.GetCount(); ++i)
		(*PropStorages.ValueAt(i))->Remove(Entity.GetUID());

	UIDToEntity.Remove(Entity.GetUID());
	Entities.RemoveAt(Idx);
}
//---------------------------------------------------------------------

void CEntityManager::DeleteEntities(const CGameLevel& Level)
{
	for (int i = Entities.GetCount() - 1; i >= 0; --i)
		if (Entities[i]->GetLevel() == &Level)
			DeleteEntity(i);
}
//---------------------------------------------------------------------

void CEntityManager::DeleteAllEntities()
{
	while (Entities.GetCount() > 0)
		DeleteEntity(*Entities.Back());
}
//---------------------------------------------------------------------

CEntity* CEntityManager::GetEntity(CStrID UID) const
{
	CEntity* pEnt = nullptr;
	UIDToEntity.Get(UID, pEnt);
	return pEnt;
}
//---------------------------------------------------------------------

void CEntityManager::GetEntitiesByLevel(const CGameLevel* pLevel, CArray<CEntity*>& Out) const
{
	for (UPTR i = 0; i < Entities.GetCount(); ++i)
	{
		CEntity* pEntity = Entities[i].Get();
		if (pEntity && pEntity->GetLevel() == pLevel)
			Out.Add(pEntity);
	}
}
//---------------------------------------------------------------------

CProperty* CEntityManager::AttachProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const
{
	if (!pRTTI) return nullptr;

	/* // Another way to get storage, without early instantiation. ???What is better?
	CPropertyStorage* pStorage = nullptr;
	const Core::CRTTI* pCurrRTTI = pRTTI;
	while (pCurrRTTI)
	{
		IPTR Idx = PropStorages.FindIndex(pRTTI);
		if (Idx != INVALID_INDEX)
		{
			pStorage = *PropStorages.ValueAt(Idx);
			break;
		}
		pCurrRTTI = pCurrRTTI->GetParent();
	}
	*/

	PProperty Prop = (CProperty*)pRTTI->CreateClassInstance();
	CPropertyStorage* pStorage = Prop->GetStorage();
	n_assert2_dbg(pStorage, (CString("Property ") + pRTTI->GetName() + " is not registered!").CStr());

	if (!pStorage->Get(Entity.GetUID(), Prop))
	{
		pStorage->Add(Entity.GetUID(), Prop);
		Prop->SetEntity(&Entity);
	}

	n_verify(Prop->Initialize());

	return Prop.Get();
}
//---------------------------------------------------------------------

void CEntityManager::RemoveProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const
{
	if (!pRTTI) return;
	IPTR Idx = PropStorages.FindIndex(pRTTI);
	n_assert2_dbg(Idx != INVALID_INDEX, (CString("Property ") + pRTTI->GetName() + " is not registered!").CStr());
	if (Idx == INVALID_INDEX) return;
	CPropertyStorage* pStorage = *PropStorages.ValueAt(Idx);
	n_assert_dbg(pStorage);

	PProperty Prop;
	if (pStorage->Get(Entity.GetUID(), Prop))
	{
		if (Prop->IsActive()) Prop->Deactivate();

		//!!!remove by iterator!
		pStorage->Remove(Entity.GetUID());
	}
}
//---------------------------------------------------------------------

CProperty* CEntityManager::GetProperty(CStrID EntityID, const Core::CRTTI* pRTTI) const
{
	if (!pRTTI || !EntityID.IsValid()) return nullptr;
	IPTR Idx = PropStorages.FindIndex(pRTTI);

	const Core::CRTTI* pStorageRTTI = pRTTI;
	while (Idx == INVALID_INDEX && pStorageRTTI && pStorageRTTI != &CProperty::RTTI)
	{
		pStorageRTTI = pStorageRTTI->GetParent();
		Idx = PropStorages.FindIndex(pStorageRTTI);
	}
	n_assert2_dbg(Idx != INVALID_INDEX, (CString("Property ") + pRTTI->GetName() + " is not registered!").CStr());
	if (Idx == INVALID_INDEX) return nullptr;

	CPropertyStorage* pStorage = *PropStorages.ValueAt(Idx);
	n_assert_dbg(pStorage);

	PProperty Prop;
	pStorage->Get(EntityID, Prop);
	return pRTTI == pStorageRTTI || Prop->IsA(*pRTTI) ? Prop.Get() : nullptr;
}
//---------------------------------------------------------------------

void CEntityManager::GetPropertiesOfEntity(CStrID EntityID, CArray<CProperty*>& Out) const
{
	for (UPTR i = 0; i < PropStorages.GetCount(); ++i)
	{
		PProperty Prop;
		if ((*PropStorages.ValueAt(i))->Get(EntityID, Prop))
			Out.Add(Prop.Get());
	}
}
//---------------------------------------------------------------------

}