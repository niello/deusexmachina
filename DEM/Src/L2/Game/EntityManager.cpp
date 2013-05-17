#include "EntityManager.h"

#include <Game/Entity.h>
#include <Events/EventManager.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntityManager, Core::CRefCounted);
__ImplementSingleton(Game::CEntityManager);

void CEntityManager::Open()
{
}
//---------------------------------------------------------------------

void CEntityManager::Close()
{
	DeleteAllEntities();

	// Delete storages allocated in RegisterProperty and clear storage refs in prop's static fields.
	// We don't need to deactivate and detach properties, DeleteAllEntities deleted all prop instances.
	for (int i = 0; i < PropStorages.GetCount(); ++i)
	{
		n_assert_dbg(!(*PropStorages.ValueAtIndex(i))->GetCount());
		n_delete(*PropStorages.ValueAtIndex(i));
		*PropStorages.ValueAtIndex(i) = NULL;
	}
	PropStorages.Clear();
}
//---------------------------------------------------------------------

PEntity CEntityManager::CreateEntity(CStrID UID, CGameLevel& Level)
{
	n_assert(UID.IsValid());
	n_assert(!EntityExists(UID)); //???return NULL or existing entity?
	PEntity Entity = n_new(CEntity(UID, Level));
	Entities.Append(Entity);
	UIDToEntity.Add(Entity->GetUID(), Entity.GetUnsafe());
	return Entity;
}
//---------------------------------------------------------------------

bool CEntityManager::RenameEntity(CEntity& Entity, CStrID NewUID)
{
	if (!NewUID.IsValid()) FAIL;
	if (Entity.GetUID() == NewUID) OK;
	if (EntityExists(NewUID)) FAIL;
	UIDToEntity.Erase(Entity.GetUID());
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
	n_error("CEntityManager::CloneEntity() -> IMPLEMENT ME!!!");
	return NULL;
}
//---------------------------------------------------------------------

void CEntityManager::DeleteEntity(int Idx)
{
	CEntity& Entity = *Entities[Idx];

	n_assert_dbg(!Entity.IsActivating() && !Entity.IsDeactivating());

	if (Entity.IsActive()) Entity.Deactivate();

	// Remove all properties of this entity
	for (int i = 0; i < PropStorages.GetCount(); ++i)
		(*PropStorages.ValueAtIndex(i))->Erase(Entity.GetUID());

	UIDToEntity.Erase(Entity.GetUID());
	Entities.EraseAt(Idx);
}
//---------------------------------------------------------------------

void CEntityManager::DeleteEntities(const CGameLevel& Level)
{
	for (int i = Entities.GetCount() - 1; i >= 0; --i)
		if (&Entities[i]->GetLevel() == &Level)
			DeleteEntity(i);
}
//---------------------------------------------------------------------

CProperty* CEntityManager::AttachProperty(CEntity& Entity, const Core::CRTTI* pRTTI) const
{
	if (!pRTTI) return NULL;

	int Idx = PropStorages.FindIndex(pRTTI);
	n_assert2_dbg(Idx != INVALID_INDEX, (nString("Property ") + pRTTI->GetName() + " is not registered!").CStr());
	if (Idx == INVALID_INDEX) return NULL;
	CPropertyStorage* pStorage = *PropStorages.ValueAtIndex(Idx);
	n_assert_dbg(pStorage);

	PProperty Prop;
	if (!pStorage->Get(Entity.GetUID(), Prop))
	{
		Prop = (CProperty*)pRTTI->Create();
		pStorage->Add(Entity.GetUID(), Prop);
		Prop->SetEntity(&Entity);
	}
	return Prop.GetUnsafe();
}
//---------------------------------------------------------------------

void CEntityManager::RemoveProperty(CEntity& Entity, Core::CRTTI& Type) const
{
	int Idx = PropStorages.FindIndex(&Type);
	n_assert2_dbg(Idx != INVALID_INDEX, (nString("Property ") + Type.GetName() + " is not registered!").CStr());
	if (Idx == INVALID_INDEX) return;
	CPropertyStorage* pStorage = *PropStorages.ValueAtIndex(Idx);
	n_assert_dbg(pStorage);
	pStorage->Erase(Entity.GetUID());
}
//---------------------------------------------------------------------

CEntity* CEntityManager::GetEntity(CStrID UID, bool SearchInAliases) const
{
	if (SearchInAliases)
	{
		int Idx = Aliases.FindIndex(UID);
		if (Idx != INVALID_INDEX) UID = Aliases.ValueAtIndex(Idx);
	}

	CEntity* pEnt = NULL;
	UIDToEntity.Get(UID, pEnt);
	return pEnt;
}
//---------------------------------------------------------------------

}