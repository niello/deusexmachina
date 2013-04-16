#pragma once
#ifndef __DEM_L2_STATIC_ENV_MANAGER_H__
#define __DEM_L2_STATIC_ENV_MANAGER_H__

#include <Data/Singleton.h> //!!!was core!
#include <Data/StringID.h>
#include <Game/Manager.h>
#include <util/ndictionary.h>

// Static environment manager manages static graphics and collision shapes without using entities
// at all thereby reducing overhead and centralising all static level geometry.

//!!!NEED DEBUG RENDERING OF COLLIDE SHAPES! foreach Shape->RenderDebug(). Gfx entities too?

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

#define StaticEnvMgr Game::CStaticEnvManager::Instance()

class CStaticEnvManager: public CManager
{
	__DeclareClass(CStaticEnvManager);
	__DeclareSingleton(CStaticEnvManager);

private:

	struct CEnvObject
	{
		nArray<Physics::PShape>			Collision;
		nArray<matrix44>				GfxLocalTfm;
		nArray<matrix44>				CollLocalTfm;
		Scene::PSceneNode				Node;
		bool							ExistingNode;
	};

	nDictionary<CStrID, CEnvObject>	EnvObjects;

public:

	CStaticEnvManager();
	virtual ~CStaticEnvManager();

	bool AddEnvObject(const DB::PValueTable& Table, int RowIdx);
	void SetEnvObjectTransform(CStrID ID, const matrix44& Tfm);
	void DeleteEnvObject(CStrID ID);
	void ClearStaticEnv();
	bool EnvObjectExists(CStrID ID) const;
	bool IsEntityStatic(CStrID ID) const;
	bool IsEntityStatic(CEntity& Entity) const;
};

}

#endif