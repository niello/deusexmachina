#pragma once
#ifndef __DEM_L2_GAME_LEVEL_H__
#define __DEM_L2_GAME_LEVEL_H__

#include <Events/EventDispatcher.h>
#include <Scene/CameraManager.h>
#include <mathlib/rectangle.h>

// Represents one game location, including all entities in it and property worlds (physics, AI, scene).
// Game server allows to perform different queries on a location.

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
	typedef Ptr<class CPhysicsWorld> PPhysicsWorld;
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
	nArray<CStrID>				SelectedEntities;

	Scene::PScene				Scene;
	Physics::PPhysicsWorld		PhysWorld;
	AI::PAILevel				AILevel;
	Scene::PCameraManager		CameraManager;

	bool OnEvent(const Events::CEventBase& Event);

public:

	~CGameLevel();

	bool					Init(CStrID LevelID, const Data::CParams& Desc);
	void					Term();
	bool					Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc = NULL);
	void					Trigger();
	void					RenderScene();
	void					RenderDebug();

	//???GetEntityAABB(AABB_Gfx | AABB_Phys);?

	// Screen queries
	bool					GetIntersectionAtScreenPos(float XRel, float YRel, vector3* pOutPoint3D = NULL, CStrID* pOutEntityUID = NULL) const;
	DWORD					GetEntitiesAtScreenRect(nArray<CEntity*>& Out, const rectangle& RelRect) const;
	bool					GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset = NULL) const;
	bool					GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const;
	bool					GetEntityScreenRect(rectangle& Out, const Game::CEntity& Entity, const vector3* Offset = NULL) const;

	// Physics-based queries
	DWORD					GetEntitiesInPhysBox(nArray<CEntity*>& Out, const matrix44& OBB) const;
	DWORD					GetEntitiesInPhysSphere(nArray<CEntity*>& Out, const vector3& Center, float Radius) const;
	bool					GetSurfaceInfoBelow(CSurfaceInfo& Out, const vector3& Position, float ProbeLength = 1000.f) const;

	//!!!fire events!
	void					AddToSelection(CStrID EntityID) { if (!IsSelected(EntityID)) SelectedEntities.Append(EntityID); }
	bool					RemoveFromSelection(CStrID EntityID) { return SelectedEntities.RemoveByValue(EntityID); }
	void					ClearSelection() { SelectedEntities.Clear(); }
	const nArray<CStrID>&	GetSelection() const { return SelectedEntities; }
	DWORD					GetSelectedCount() const { return SelectedEntities.GetCount(); }
	bool					IsSelected(CStrID EntityID) const { return SelectedEntities.Contains(EntityID); }

	CStrID					GetID() const { return ID; }
	const nString&			GetName() const { return Name; }

	Scene::CScene*			GetScene() const { return Scene.GetUnsafe(); }
	Physics::CPhysicsWorld*	GetPhysics() const { return PhysWorld.GetUnsafe(); }
	AI::CAILevel*			GetAI() const { return AILevel.GetUnsafe(); }
	Scene::CCameraManager*	GetCameraMgr() const { return CameraManager.GetUnsafe(); }
};

typedef Ptr<CGameLevel> PGameLevel;

}

#endif
