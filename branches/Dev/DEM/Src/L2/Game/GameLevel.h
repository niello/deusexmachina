#pragma once
#ifndef __DEM_L2_GAME_LEVEL_H__
#define __DEM_L2_GAME_LEVEL_H__

#include <Events/EventDispatcher.h>
#include <mathlib/rectangle.h>

// Represents one game location, including all entities in it and property worlds (physics, AI, scene).
// Game server allows to perform different queries on a location.

//???need entity quadtree? or always use physics or mesh AABBs and respective world's spatial query

namespace Data
{
	class CParams;
}

namespace Scripting
{
	typedef Ptr<class CScriptObject> PScriptObject;
}

namespace Scene
{
	typedef Ptr<class CScene> PScene;
}

namespace Physics
{
	typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
	typedef int CMaterialType;
}

namespace AI
{
	typedef Ptr<class CAILevel> PAILevel;
}

namespace Game
{
class CEntity;

struct CSurfaceInfo
{
	//float					TerrainHeight;
	float					WorldHeight;
	Physics::CMaterialType	Material;
	//???where to ignore dynamic/AI objects, where not to?
	//???how to check multilevel ground (bridge above a road etc)?
	//???how to check is point inside world geom?
};

class CGameLevel: public Events::CEventDispatcher
{
protected:

	CStrID						ID;
	nString						Name;
	Events::PSub				GlobalSub;
	Scripting::PScriptObject	Script;

	Scene::PScene				Scene;
	Physics::PPhysicsLevel		PhysicsLevel;
	AI::PAILevel				AILevel;

	bool OnEvent(const Events::CEventBase& Event);

public:

	~CGameLevel();

	bool			Init(CStrID LevelID, const Data::CParams& Desc);
	void			Term();
	void			Trigger();
	void			RenderScene();
	void			RenderDebug();

	//???GetEntityAABB(AABB_Gfx | AABB_Phys);?

	// Screen queries
	CEntity*		GetEntityAtScreenPos(float RelX, float RelY) const; //???write 2 versions, physics-based and mesh-based?
	DWORD			GetEntitiesAtScreenRect(nArray<CEntity*>& Out, const rectangle& RelRect) const;
	bool			GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset = NULL) const;
	bool			GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const;
	bool			GetEntityScreenRect(rectangle& Out, const Game::CEntity& Entity, const vector3* Offset = NULL) const;

	// Physics-based queries
	DWORD			GetEntitiesInPhysBox(nArray<CEntity*>& Out, const matrix44& OBB) const;
	DWORD			GetEntitiesInPhysSphere(nArray<CEntity*>& Out, const vector3& Center, float Radius) const;
	bool			GetSurfaceInfoUnder(CSurfaceInfo& Out, const vector3& Position, float ProbeLength = 1000.f /*, //!!!FILTER!*/) const;

	CStrID			GetID() const { return ID; }
	const nString&	GetName() const { return Name; }

	Scene::CScene*	GetScene() const { return Scene.GetUnsafe(); }
	Physics::CPhysicsLevel*	GetPhysics() const { return PhysicsLevel.GetUnsafe(); }
	AI::CAILevel*	GetAI() const { return AILevel.GetUnsafe(); }
};

typedef Ptr<CGameLevel> PGameLevel;

}

#endif
