#include "EntityManager.h"

#include <Game/Entity.h>
#include <Game/GameServer.h>
#include <Physics/PhysicsServer.h>
#include <Loading/EntityFactory.h>
#include <Events/EventManager.h>

namespace Game
{
ImplementRTTI(Game::CEntityManager, Game::CManager);
ImplementFactory(Game::CEntityManager);

CEntityManager* CEntityManager::Singleton = NULL;

CEntityManager::CEntityManager() :
	Entities(256, 256),
	EntityRegistry(1024),
	IsInOnFrame(false)
{
	n_assert(!Singleton);
	Singleton = this;

	PROFILER_INIT(profOnBeginFrame, "profMangaEntityManagerBeginFrame");
	PROFILER_INIT(profOnMoveBefore, "profMangaEntityManagerMoveBefore");
	PROFILER_INIT(profPhysics, "profMangaEntityManagerPhysics");
	PROFILER_INIT(profOnMoveAfter, "profMangaEntityManagerMoveAfter");
	PROFILER_INIT(profOnRender, "profMangaEntityManagerRender");
	PROFILER_INIT(profFrame, "profMangaEntityManagerFrame");
	PROFILER_INIT(profUpdateRegistry, "profMangaEntityManagerUpdateRegistry");
}
//---------------------------------------------------------------------

CEntityManager::~CEntityManager()
{
	n_assert(!Entities.Size());
	n_assert(!EntityRegistry.Size());
	n_assert(Singleton);
	Singleton = NULL;
}
//---------------------------------------------------------------------

void CEntityManager::Activate()
{
	CManager::Activate();
	SUBSCRIBE_PEVENT(OnFrame, CEntityManager, OnFrame);
}
//---------------------------------------------------------------------

void CEntityManager::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnFrame);
	CManager::Deactivate();
}
//---------------------------------------------------------------------

void CEntityManager::AddEntityToReqistry(CEntity* pEntity)
{
	n_assert(pEntity);

	// do not add the entity directly, to prevent a partly OnFrame trigger
	if (IsInOnFrame) NewEntities.Append(pEntity);
	else
	{
		Entities.Append(pEntity);
		EntityRegistry.Add(pEntity->GetUniqueID(), pEntity);
	}
}
//---------------------------------------------------------------------

// To make it possible to remove a entity while looping over the entity array
// only the ptr will set to 0 (to not change the array layout).
// Those 0 ptr's will be ignored in the OnFrame loops, and cleared on end of frame.
void CEntityManager::RemoveEntityFromRegistry(CEntity* pEntity)
{
	n_assert(pEntity);

	if (IsInOnFrame)
	{
		// [mse] 16.05.2006
		// look first in new Entities, if entity was added in the same frame
		nArray<PEntity>::iterator Iter = NewEntities.Find(pEntity);
		if (Iter) NewEntities.Erase(Iter);
		else
		{
			// save removed entity in array (to make sure the entity does not get destroyed until end of frame)
			// inside OnFrame, just set the ptrs to 0, so the entity is not longer triggered
			RemovedEntities.Append(pEntity);
			EntityRegistry.Erase(pEntity->GetUniqueID());
			//EntityRegistry[pEntity->GetUniqueID()] = NULL;
			Iter = Entities.Find(pEntity);
			n_assert(Iter);
			(*Iter) = NULL;
		}
	}
	else
	{
		// directly remove entity from arrays
		EntityRegistry.Erase(pEntity->GetUniqueID());
		nArray<PEntity>::iterator Iter = Entities.Find(pEntity);
		n_assert(Iter);
		Entities.Erase(Iter);
	}
}
//---------------------------------------------------------------------

void CEntityManager::UpdateRegistry()
{
	RemovedEntities.Clear();

	//for (int i = 0; i < EntityRegistry.Size(); /*empty*/)
	//	if (EntityRegistry.GetElementAt(i)) i++;
	//	else EntityRegistry.RemByIndex(i);

	// remove 0 ptrs from entity array and add new Entities
	for (int i = 0; i < Entities.Size(); /*empty*/)
	{
		if (Entities[i].isvalid()) i++;
		else
		{
			if (NewEntities.Size() > 0)
			{
				// move one of the new entity at this 0 ptr
				Entities[i] = NewEntities.Back();
				NewEntities.Erase(NewEntities.Size() - 1);
				EntityRegistry.Add(Entities[i]->GetUniqueID(), Entities[i]);
				i++;
			}
			else Entities.Erase(i);
		}
	}

	while (NewEntities.Size() > 0)
	{
		Entities.Append(NewEntities.Back());
		NewEntities.Erase(NewEntities.Size() - 1);
		EntityRegistry.Add(Entities.Back()->GetUniqueID(), Entities.Back());
	}
}
//---------------------------------------------------------------------

void CEntityManager::AttachEntity(CEntity* pEntity, bool AutoActivate)
{
	n_assert(pEntity && pEntity->GetUniqueID().IsValid());
	AddEntityToReqistry(pEntity);
	if (AutoActivate) ActivateEntity(pEntity);
}
//---------------------------------------------------------------------

void CEntityManager::ActivateEntity(CEntity* pEntity)
{
	//???!!!assert is added?!
	n_assert(pEntity && pEntity->GetUniqueID().IsValid());
	pEntity->Activate();
	if (GameSrv->HasStarted()) pEntity->FireEvent(CStrID("OnStart"));
}
//---------------------------------------------------------------------

// Remove a game entity from the entity manager. This basically removes the entity from the world.
void CEntityManager::RemoveEntity(CEntity* pEntity)
{
	n_assert(pEntity);
	pEntity->Deactivate();
	RemoveEntityFromRegistry(pEntity);
}
//---------------------------------------------------------------------

// Delete a game entity, this removes the entity from the world and also deletes the DB record of this entity.
void CEntityManager::DeleteEntity(CEntity* pEntity)
{
	n_assert(pEntity);

	// to make sure, the entity still exists, when removing from level
	PEntity LastPtr = pEntity;

	pEntity->FireEvent(CStrID("OnDelete"));
	RemoveEntity(pEntity);
	EntityFct->DeleteEntityInstance(pEntity); //!!!order & responsibility of creation-destruction are different!
}
//---------------------------------------------------------------------

bool CEntityManager::ChangeEntityID(PEntity Entity, CStrID NewID)
{
	if (!Entity.isvalid() || !NewID.IsValid()) FAIL;
	if (Entity->GetUniqueID() == NewID) OK;
	if (ExistsEntityByID(NewID)) FAIL;
	RemoveEntityFromRegistry(Entity);
	EntityFct->RenameEntityInstance(Entity, NewID);
	Entity->SetUniqueID(NewID);
	AddEntityToReqistry(Entity);
	OK;
}
//---------------------------------------------------------------------

void CEntityManager::RemoveAllEntities()
{
	n_assert(!IsInOnFrame);
	while (Entities.Size() > 0) RemoveEntity(Entities.Back());
}
//---------------------------------------------------------------------

bool CEntityManager::SetEntityAlias(CStrID Alias, CStrID UID)
{
	if (!UID.IsValid()) FAIL;
	int Idx = Aliases.FindIndex(Alias);
	if (Idx != -1) Aliases.ValueAtIndex(Idx) = UID;
	else Aliases.Add(Alias, UID);
	OK;
}
//---------------------------------------------------------------------

void CEntityManager::RemoveEntityAlias(CStrID Alias)
{
	int Idx = Aliases.FindIndex(Alias);
	if (Idx != -1) Aliases.EraseAt(Idx);
}
//---------------------------------------------------------------------

bool CEntityManager::OnFrame(const Events::CEventBase& Event)
{
	PROFILER_START(profFrame);
	n_assert(!IsInOnFrame);
	IsInOnFrame = true;

	int entityIndex;
	int numEntities = Entities.Size();

#if DEM_STATS
	int numLiveEntities = 0;
	int numSleepingEntities = 0;
	for (entityIndex = 0; entityIndex < numEntities; entityIndex++)
		if (Entities[entityIndex]->GetPool() == LivePool) numLiveEntities++;
		else numSleepingEntities++;
	//statsNumEntities->SetValue(numEntities);
	//statsNumLiveEntities->SetValue(numLiveEntities);
	//statsNumSleepingEntities->SetValue(numSleepingEntities);
#endif

	PROFILER_START(profOnBeginFrame);
	EventMgr->FireEvent(CStrID("OnBeginFrame"));
	PROFILER_STOP(profOnBeginFrame);

	PROFILER_START(profOnMoveBefore);
	EventMgr->FireEvent(CStrID("OnMoveBefore"));
	PROFILER_STOP(profOnMoveBefore);

	PROFILER_START(profPhysics);
	PhysicsSrv->Trigger();
	PROFILER_STOP(profPhysics);

	PROFILER_START(profOnMoveAfter);
	EventMgr->FireEvent(CStrID("OnMoveAfter"));
	PROFILER_STOP(profOnMoveAfter);

	PROFILER_START(profOnRender);
	EventMgr->FireEvent(CStrID("OnRender"));
	PROFILER_STOP(profOnRender);

	n_assert(IsInOnFrame);
	IsInOnFrame = false;

	PROFILER_START(profUpdateRegistry);
	UpdateRegistry();
	PROFILER_STOP(profUpdateRegistry);

	PROFILER_STOP(profFrame);

	OK;
}
//---------------------------------------------------------------------

CEntity* CEntityManager::GetEntityByID(CStrID UID, bool SearchInAliases) const
{
	CEntity* pEnt = NULL;

	if (SearchInAliases)
	{
		int Idx = Aliases.FindIndex(UID);
		if (Idx != -1) UID = Aliases.ValueAtIndex(Idx);
	}

	EntityRegistry.Get(UID, pEnt);
	return (pEnt && !pEnt->IsLive()) ? NULL : pEnt;
}
//---------------------------------------------------------------------

//bool CEntityManager::ExistsEntityByAttr(const DB::CAttr& Attr, bool LiveOnly) const
//{
//    nArray<DB::CAttr> Attrs;
//    Attrs.Append(Attr);
//    return ExistsEntitiesByAttrs(Attrs, LiveOnly);
//}
////---------------------------------------------------------------------

//nArray<PEntity> CEntityManager::GetEntitiesByAttr(const DB::CAttr& Attr,
//												  bool LiveOnly,
//												  bool FirstOnly,
//												  bool FailOnDBError)
//{
//    nArray<DB::CAttr> Attrs;
//    Attrs.Append(Attr);
//    return GetEntitiesByAttrs(Attrs, LiveOnly, FirstOnly, FailOnDBError);
//}
////---------------------------------------------------------------------

/*
bool CEntityManager::ExistsEntitiesByAttrs(const nArray<DB::CAttr>& Attrs, bool LiveOnly) const
{
    // search in the active Entities
    int entityIndex;
    int numEntities = GetNumEntities();
    for (entityIndex = 0; entityIndex < numEntities; entityIndex++)
    {
        CEntity* entity = GetEntityAt(entityIndex);
        // entity exist?
        if (entity)
        {
            // is in the right pool?
            if (!LiveOnly || entity->GetPool() == LivePool)
            {
                // has all attribute?
                int attributeIndex;
                bool hasAllAttributes = true;
                for (attributeIndex = 0; attributeIndex < Attrs.Size(); attributeIndex++)
                {
                    if (!entity->HasAttr(Attrs[attributeIndex].GetAttrID())
                        || (entity->GetAttr(Attrs[attributeIndex].GetAttrID()) != Attrs[attributeIndex]))
                    {
                        hasAllAttributes = false;
                        break;
                    }
                }

                if (hasAllAttributes)
                {
                    return true; // found one
                }
            }
        }
    }

    // search in the db
    Ptr<DB::Query> dbQuery = DBSrv->CreateQuery();
    dbQuery->SetTableName("_Entities");
    dbQuery->AddWhereAttr(DB::CAttr(Attr::_Type, nString("INSTANCE")));

    // add Attrs as where clause
    for (int i = 0; i < Attrs.Size(); i++)
    {
        dbQuery->AddWhereAttr(Attrs[i]);
    }
    // define results
    dbQuery->AddResultAttr(Attr::GUID);
    dbQuery->BuildSelectStatement();

    if (dbQuery->Execute())
    {
        return dbQuery->GetRowCount() > 0;
    }

    // fall through, something went wrong
    nString err;
    for (int i = 0; i < Attrs.Size(); i++)
    {
        err.Append(Attrs[i].GetName());
        err.Append("=");
        err.Append(Attrs[i].AsString());
        err.Append(" ");
    }
    n_error("Managers::CEntityManager::ExistsEntitiesByAttrs(): failed to execute query with keys '%s' into world database!", err.Get());
    return false;
}

//------------------------------------------------------------------------------
    //Generic function to find Entities by Attrs.
    //@param FirstOnly  set to stop search if the 1st entity was found

nArray<PEntity>
CEntityManager::GetEntitiesByAttrs(const nArray<DB::CAttr>& Attrs, bool LiveOnly, bool FirstOnly, bool FailOnDBError)
{
    // collect active Entities
    nArray<PEntity> Entities;
    int entityIndex;
    int numEntities = GetNumEntities();
    for (entityIndex = 0; entityIndex < numEntities; entityIndex++)
    {
        CEntity* entity = GetEntityAt(entityIndex);
        // entity exist?
        if (entity)
        {
            // is in the right pool?
            if (!LiveOnly || entity->GetPool() == LivePool)
            {
                // has all attribute?
                int attributeIndex;
                bool hasAllAttributes = true;
                for (attributeIndex = 0; attributeIndex < Attrs.Size(); attributeIndex++)
                {
                    if (!entity->HasAttr(Attrs[attributeIndex].GetAttrID())
                        || (entity->GetAttr(Attrs[attributeIndex].GetAttrID()) != Attrs[attributeIndex]))
                    {
                        hasAllAttributes = false;
                        break;
                    }
                }

                if (hasAllAttributes)
                {
                    Entities.Append(entity);
                    if (FirstOnly)
                    {
                        return Entities;
                    }
                }
            }
        }
    }

    if (!LiveOnly)
    {
        // get Entities from db
        nArray<CEntity*> dbEntities = CreateSleepingEntities(Attrs, Entities, FailOnDBError);
        for (int i = 0; i < dbEntities.Size(); i++)
        {
            Entities.Append(dbEntities[i]);
        }
    }

    return Entities;
}

//------------------------------------------------------------------------------
    //This method creates a new sleeping Entities from the database using the
    //provided key Attrs (usually Attr::Name or Attr::GUID) and attaches
    //it to the world. Please note that the method may return a array with 0
    //elements if such Entities are not found in the database!

    //If there are filteredEntities those will be filtered in the DB query by
    //GUID.
nArray<CEntity*>
CEntityManager::CreateSleepingEntities(const nArray<DB::CAttr>& keyAttributes, const nArray<Ptr<Game::CEntity> >& filteredEntities, bool FailOnDBError)
{
    nArray<CEntity*> Entities = CEntityFactory::Instance()->CreateEntitiesByKeyAttrs(keyAttributes, filteredEntities, SleepingPool, FailOnDBError);
    for (int i = 0; i < Entities.Size(); i++)
    {
        AttachEntity(Entities[i]);
    }
    return Entities;
}
//---------------------------------------------------------------------
*/

}