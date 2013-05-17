#pragma once
#ifndef __DEM_L2_STATIC_ENV_MANAGER_H__
#define __DEM_L2_STATIC_ENV_MANAGER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
#include <Data/StringID.h>
#include <util/ndictionary.h>

// Static environment manager manages static environment. It includes graphics and collision shapes,
// that are not simulated / animated / controlled, so there is no point in storing them as separate
// entities. Static environment can't store per-entity attributes and can't be extended by properties.
// If this manager founds that some entity can't be added as static, it rejects it. (???or forward to regulat entity mgr?)

//!!!DEBUG RENDER OF ACTIVE LEVEL!

namespace Physics
{
	typedef Ptr<class CShape> PShape;
}

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace DB
{
	typedef Ptr<class CValueTable> PValueTable;
}

namespace Game
{
using namespace Data;

class CEntity;
class CGameLevel;

#define StaticEnvMgr Game::CStaticEnvManager::Instance()

class CStaticEnvManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CStaticEnvManager);

private:

	struct CEnvObject
	{
		nArray<Physics::PShape>			Collision; //???scene node attributes?
		nArray<matrix44>				CollLocalTfm; //???use child scene node?
		Scene::PSceneNode				Node;
		bool							ExistingNode;
	};

	nDictionary<CStrID, CEnvObject>	EnvObjects;

public:

	CStaticEnvManager() { __ConstructSingleton; }
	~CStaticEnvManager() { __DestructSingleton; }

	bool CreateStaticObject(CStrID UID, CGameLevel& Level);
	void SetEnvObjectTransform(CStrID ID, const matrix44& Tfm);
	void DeleteEnvObject(CStrID ID);
	void ClearStaticEnv();
	bool EnvObjectExists(CStrID ID) const;
	bool IsEntityStatic(CStrID ID) const;
	bool IsEntityStatic(CEntity& Entity) const;
};

}

#endif