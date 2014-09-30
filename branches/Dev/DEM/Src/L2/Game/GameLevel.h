#pragma once
#ifndef __DEM_L2_GAME_LEVEL_H__
#define __DEM_L2_GAME_LEVEL_H__

#include <Events/EventDispatcher.h>
#include <Render/SPS.h>
#include <Scene/CameraManager.h> //!!!not Scene!
#include <Render/Camera.h>
#include <Math/Rect.h>

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

// Information about a world surface at the given point/region
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
	CString						Name;
	CAABB						BBox;			// Now primarily for culling through a spatial partitioning structure
	Events::PSub				GlobalSub;
	Scripting::PScriptObject	Script;
	CArray<CStrID>				SelectedEntities;

	bool						AutoAdjustCameraAspect;

	//!!!to World::CNatureManager or smth like (weather, time of day etc) and set to the render server directly!
	vector4						AmbientLight;
	//Fog settings
	//???shadow settings?

	Scene::PSceneNode			SceneRoot;
	Render::CSPS				SPS;			// Spatial partitioning structure
	Physics::PPhysicsWorld		PhysWorld;
	AI::PAILevel				AILevel;
	Scene::PCameraManager		CameraManager;	//???!!!Render::?! //???manage cameras here in a level itself?

	//!!!???if PCameraManager is really useful, move it there!
	Render::PCamera				MainCamera;

	bool OnEvent(const Events::CEventBase& Event);

public:

	CGameLevel(): AmbientLight(0.2f, 0.2f, 0.2f, 1.f), AutoAdjustCameraAspect(true) {}
	virtual ~CGameLevel();

	bool					Init(CStrID LevelID, const Data::CParams& Desc);
	void					Term();
	bool					Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc = NULL);
	void					Trigger();
	void					RenderScene();
	void					RenderDebug();

	//???GetEntityAABB(AABB_Gfx | AABB_Phys);?

	//!!!ensure there are SPS-accelerated queries!
	// Screen queries
	bool					GetIntersectionAtScreenPos(float XRel, float YRel, vector3* pOutPoint3D = NULL, CStrID* pOutEntityUID = NULL) const;
	DWORD					GetEntitiesAtScreenRect(CArray<CEntity*>& Out, const rectangle& RelRect) const;
	bool					GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset = NULL) const;
	bool					GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const;
	bool					GetEntityScreenRect(rectangle& Out, const Game::CEntity& Entity, const vector3* Offset = NULL) const;

	// Physics-based queries
	DWORD					GetEntitiesInPhysBox(CArray<CEntity*>& Out, const matrix44& OBB) const;
	DWORD					GetEntitiesInPhysSphere(CArray<CEntity*>& Out, const vector3& Center, float Radius) const;
	bool					GetSurfaceInfoBelow(CSurfaceInfo& Out, const vector3& Position, float ProbeLength = 1000.f) const;

	void					AddToSelection(CStrID EntityID);
	bool					RemoveFromSelection(CStrID EntityID);
	void					ClearSelection() { SelectedEntities.Clear(); }
	const CArray<CStrID>&	GetSelection() const { return SelectedEntities; }
	DWORD					GetSelectedCount() const { return SelectedEntities.GetCount(); }
	bool					IsSelected(CStrID EntityID) const { return SelectedEntities.Contains(EntityID); }

	CStrID					GetID() const { return ID; }
	const CString&			GetName() const { return Name; }

	//!!!GetSceneNode() const and non-const variants w/ and w/out creation!
	Scene::CSceneNode*		GetSceneNode(LPCSTR Path, bool Create = false) { return SceneRoot->GetChild(Path, Create); }
	Scene::CSceneNode*		GetSceneRoot() { return SceneRoot.GetUnsafe(); }
	Physics::CPhysicsWorld*	GetPhysics() const { return PhysWorld.GetUnsafe(); }
	AI::CAILevel*			GetAI() const { return AILevel.GetUnsafe(); }
	Scene::CCameraManager*	GetCameraMgr() const { return CameraManager.GetUnsafe(); }
};

typedef Ptr<CGameLevel> PGameLevel;

}

#endif
