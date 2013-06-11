#pragma once
#ifndef __DEM_L2_STATIC_ENV_MANAGER_H__
#define __DEM_L2_STATIC_ENV_MANAGER_H__

#include <Game/StaticObject.h>
#include <Core/Singleton.h>
#include <util/ndictionary.h>

// Static environment manager manages static objects. It includes graphics and collision shapes
// that aren't required to be stored as separate entities. Static environment can't store per-entity
// attributes and can't be extended by properties.

//!!!DEBUG RENDER OF ACTIVE LEVEL!

namespace Game
{
#define StaticEnvMgr Game::CStaticEnvManager::Instance()

class CStaticEnvManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CStaticEnvManager);

private:

	nDictionary<CStrID, PStaticObject> Objects;

	void DeleteStaticObject(int Idx);

public:

	CStaticEnvManager() { __ConstructSingleton; }
	~CStaticEnvManager() { DeleteAllStaticObjects(); __DestructSingleton; }

	bool			CanEntityBeStatic(const Data::CParams& Desc) const;

	PStaticObject	CreateStaticObject(CStrID UID, CGameLevel& Level);
	bool			RenameStaticObject(CStaticObject& Obj, CStrID NewUID);
	PStaticObject	CloneStaticObject(const CStaticObject& Obj, CStrID UID);
	void			DeleteStaticObject(CStaticObject& Obj);
	void			DeleteStaticObject(CStrID UID);
	void			DeleteStaticObjects(const CGameLevel& Level);
	void			DeleteAllStaticObjects();

	int				GetStaticObjectCount() const { return Objects.GetCount(); }
	CStaticObject*	GetStaticObject(int Idx) const { return Objects.ValueAt(Idx).GetUnsafe(); }
	CStaticObject*	GetStaticObject(CStrID UID) const;
	bool			StaticObjectExists(CStrID UID) const { return !!GetStaticObject(UID); }
};

typedef Ptr<CStaticEnvManager> PStaticEnvManager;

inline CStaticObject* CStaticEnvManager::GetStaticObject(CStrID UID) const
{
	int Idx = Objects.FindIndex(UID);
	return (Idx != INVALID_INDEX) ? Objects.ValueAt(Idx).GetUnsafe() : NULL;
}
//---------------------------------------------------------------------

inline void CStaticEnvManager::DeleteStaticObject(CStaticObject& Entity)
{
	//!!!find idx by value!
}
//---------------------------------------------------------------------

inline void CStaticEnvManager::DeleteStaticObject(CStrID UID)
{
	int Idx = Objects.FindIndex(UID);
	if (Idx != INVALID_INDEX) DeleteStaticObject(Idx);
}
//---------------------------------------------------------------------

}

#endif