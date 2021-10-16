#pragma once
#include <Data/RefCounted.h>
#include <Data/Regions.h>
#include <Game/ECS/Entity.h>
#include <Scene/SPS.h>
#include <Math/AABB.h>

// Represents one game location. Consists of subsystem worlds (scene, graphics, physics, AI).
// In MVC pattern it is a model.

namespace Scene
{
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace Physics
{
	typedef Ptr<class CPhysicsLevel> PPhysicsLevel;
	class CPhysicsObject;
}

namespace AI
{
	typedef Ptr<class CAILevel> PAILevel;
}

namespace DEM::AI
{
	using PNavMap = Ptr<class CNavMap>;
}

namespace Resources
{
	class CResourceManager;
	typedef Ptr<class CResource> PResource;
}

namespace Data
{
	class CParams;
}

namespace DEM::Game
{
typedef Ptr<class CGameLevel> PGameLevel;

class CGameLevel : public Data::CRefCounted
{
protected:

	CStrID                 _ID;

	Scene::PSceneNode      _SceneRoot;
	Scene::CSPS            _SPS;
	Physics::PPhysicsLevel _PhysicsLevel;
	::AI::PAILevel         _AILevel;

	std::vector<DEM::AI::PNavMap> _NavMaps; // Sorted by R & H

public:

	static PGameLevel LoadFromDesc(CStrID ID, const Data::CParams& In, Resources::CResourceManager& ResMgr);

	CGameLevel(CStrID ID, const CAABB& Bounds, const CAABB& InteractiveBounds = CAABB::Empty, UPTR SubdivisionDepth = 0);
	virtual ~CGameLevel() override;

	bool                     Validate(Resources::CResourceManager& RsrcMgr);
	void                     Update(float dt, const vector3* pCOIArray, UPTR COICount);

	void                     SetNavRegionController(CStrID RegionID, HEntity Controller);
	void                     SetNavRegionFlags(CStrID RegionID, U16 Flags, bool On);

	Physics::CPhysicsObject* GetFirstPickIntersection(const line3& Ray, vector3* pOutPoint3D = nullptr) const;
	// Query hierarchy:
	// 1. All physics objects in a shape
	// 2. Physics objects in a shape filtered by the collision group
	// 3. Entities in a shape optionally filtered by the collision group
	// 4. Reachable entities in a shape (navigation)
	// 5. Reachable entities in a shape filtered by a custom filter, e.g. by a component presence
	//???2 methods (physics objects & entities) with filters applied stage by stage according to flags?
	//collision group filtering is free, it occurs anyway, with "All" by default
	//ray check may need to apply collision group filter after the raycast, e.g. when searching for
	//the closest Interactable, if the user wants closer non-interactable collision objects to block it.
	//???is trigger/probe a subclass of CPhysicsObject?
	UPTR                     EnumEntitiesInSphere(const vector3& Position, float Radius, CStrID CollisionMask, std::function<bool(HEntity&, const vector3&)>&& Callback) const;

	CStrID                   GetID() const { return _ID; }

	Scene::CSceneNode&       GetSceneRoot() { return *_SceneRoot.Get(); }
	Scene::CSPS&             GetSPS() { return _SPS; }
	Physics::CPhysicsLevel*  GetPhysics() const { return _PhysicsLevel.Get(); }
	::AI::CAILevel*          GetAI() const { return _AILevel.Get(); }
	DEM::AI::CNavMap*        GetNavMap(float AgentRadius, float AgentHeight) const;
};

}


//////////////// TODO: REMOVE ///////////////////////////////
#include <Events/EventDispatcher.h>

namespace Data
{
	class CParams;
}

namespace Scripting
{
	typedef Ptr<class CScriptObject> PScriptObject;
}

namespace Game
{
class CEntity;

// Information about a world surface at the given point/region
struct CSurfaceInfo
{
	//float					TerrainHeight;
	float					WorldHeight;
	// Physics material or at least its ID
	//???where to ignore dynamic/AI objects, where not to?
	//???how to check multilevel ground (bridge above a road etc)?
	//???how to check is point inside world geom?
};

class CGameLevel: public Events::CEventDispatcher, public Data::CRefCounted
{
protected:

	CStrID						ID;
	CString						Name;
	Events::PSub				GlobalSub;
	Scripting::PScriptObject	Script;

	Scene::PSceneNode			SceneRoot;
	Scene::CSPS					SPS;			// Spatial partitioning structure
	Physics::PPhysicsLevel		PhysicsLevel;
	AI::PAILevel				AILevel;

public:

	CGameLevel();
	virtual ~CGameLevel();

	bool					Load(CStrID LevelID, const Data::CParams& Desc);
	void					Term();
	bool					Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc = nullptr);
	//void					RenderDebug();

	//!!!ensure there are SPS-accelerated queries!
	// Screen queries
	//???pass camera? or move to view? here is more universal, but may need renaming as there is no "screen" at the server part
	//can add shortcut methods to a View, with these names, calling renamed level methods with a view camera
	bool					GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset = nullptr) const;
	bool					GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const;

	CStrID					GetID() const { return ID; }
	const CString&			GetName() const { return Name; }

	Scene::CSceneNode*		GetSceneRoot() { return SceneRoot.Get(); }
	Scene::CSPS&			GetSPS() { return SPS; }
	Physics::CPhysicsLevel*	GetPhysics() const { return PhysicsLevel.Get(); }
	AI::CAILevel*			GetAI() const { return AILevel.Get(); }
};

}
