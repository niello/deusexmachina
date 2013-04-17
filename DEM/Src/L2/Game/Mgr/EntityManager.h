#pragma once
#ifndef __DEM_L2_ENTITY_MANAGER_H__
#define __DEM_L2_ENTITY_MANAGER_H__

#include <Game/Manager.h>
#include <Game/Entity.h>
#include <Events/Events.h>
#include <util/ndictionary.h>
#include <Debug/Profiler.h>

// The entity manager object keeps track of all active game entities
// and fires per-frame-update events to keep them alive. It
// also contains methods to iterate through existing entities.
// Based on mangalore EntityManager_(C) 2005 Radon Labs GmbH

namespace Game
{
using namespace Core;

#define EntityMgr Game::CEntityManager::Instance()

class CEntityManager: public CManager
{
	DeclareRTTI;
	DeclareFactory(CEntityManager);

protected:

	static CEntityManager* Singleton;

	bool						IsInOnFrame;

	nArray<PEntity>				Entities;
	nArray<PEntity>				NewEntities;
	nArray<PEntity>				RemovedEntities;

	CHashTable<CStrID, CEntity*>	EntityRegistry;
	nDictionary<CStrID, CStrID>	Aliases; //???map? of CEntity*?

	PROFILER_DECLARE(profOnBeginFrame);
	PROFILER_DECLARE(profOnMoveBefore);
	PROFILER_DECLARE(profPhysics);
	PROFILER_DECLARE(profOnMoveAfter);
	PROFILER_DECLARE(profOnRender);
	PROFILER_DECLARE(profFrame);
	PROFILER_DECLARE(profUpdateRegistry);

	void AddEntityToReqistry(CEntity* pEntity);
	void RemoveEntityFromRegistry(CEntity* pEntity);
	void UpdateRegistry();

	DECLARE_EVENT_HANDLER(OnFrame, OnFrame);

public:

	CEntityManager();
	virtual ~CEntityManager();

	static CEntityManager* Instance() { n_assert(Singleton); return Singleton; }

	virtual void	Activate();
	virtual void	Deactivate();

	void			AttachEntity(CEntity* pEntity, bool AutoActivate = true);
	void			ActivateEntity(CEntity* pEntity);
	void			RemoveEntity(CEntity* pEntity);
	void			RemoveEntity(CStrID UID);
	void			DeleteEntity(CEntity* pEntity);
	void			DeleteEntity(CStrID UID);
	bool			ChangeEntityID(PEntity Entity, CStrID NewID);
	void			RemoveAllEntities();

	bool			SetEntityAlias(CStrID Alias, CStrID UID);
	void			RemoveEntityAlias(CStrID Alias);

	int				GetNumEntities() const { return Entities.Size(); } //!!!including NULL entities!
	CEntity*		GetEntityAt(int index) const { return Entities[index].get_unsafe(); }
	const nArray<PEntity>& GetEntities() const { return Entities; }

	bool			ExistsEntityByID(CStrID UID) const;
	CEntity*		GetEntityByID(CStrID UID, bool SearchInAliases = false) const;

	bool			ExistsEntityByAttr(CAttrID AttrID, const CData& Value, bool LiveOnly = false) const;
	//bool			ExistsEntitiesByAttrs(const nArray<DB::CAttr>& Attrs, bool LiveOnly = false) const;
	nArray<PEntity>	GetEntitiesByAttr(CAttrID AttrID, const CData& Value, bool LiveOnly = false, bool FirstOnly = false, bool FailOnDBError = true);
	//nArray<PEntity> GetEntitiesByAttrs(const nArray<DB::CAttr>& Attrs, bool LiveOnly = false, bool FirstOnly = false, bool FailOnDBError = true);
};
//---------------------------------------------------------------------

RegisterFactory(CEntityManager);

inline bool CEntityManager::ExistsEntityByID(CStrID UID) const
{
	CEntity* pEnt = NULL;
	return (EntityRegistry.Get(UID, pEnt) && pEnt && pEnt->IsLive());
}
//---------------------------------------------------------------------

inline void CEntityManager::RemoveEntity(CStrID UID)
{
	CEntity* pEnt = GetEntityByID(UID, true);
	if (pEnt) RemoveEntity(pEnt);
}
//---------------------------------------------------------------------

inline void CEntityManager::DeleteEntity(CStrID UID)
{
	CEntity* pEnt = GetEntityByID(UID, true);
	if (pEnt) DeleteEntity(pEnt);
}
//---------------------------------------------------------------------

}

#endif
