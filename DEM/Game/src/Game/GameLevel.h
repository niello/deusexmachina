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

	bool                     Validate(Resources::CResourceManager& ResMgr);
	void                     Update(float dt, const vector3* pCOIArray, UPTR COICount);

	void                     SetNavRegionController(CStrID RegionID, HEntity Controller);

	Physics::CPhysicsObject* GetFirstPickIntersection(const line3& Ray, vector3* pOutPoint3D = nullptr) const;

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

	bool OnEvent(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

public:

	CGameLevel();
	virtual ~CGameLevel();

	bool					Load(CStrID LevelID, const Data::CParams& Desc);
	bool					Validate(Resources::CResourceManager& ResMgr);
	void					Term();
	bool					Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc = nullptr);
	//void					RenderDebug();

	//!!!ensure there are SPS-accelerated queries!
	// Screen queries
	//???pass camera? or move to view? here is more universal, but may need renaming as there is no "screen" at the server part
	//can add shortcut methods to a View, with these names, calling renamed level methods with a view camera
	UPTR					GetEntitiesAtScreenRect(CArray<CEntity*>& Out, const Data::CRect& RelRect) const;
	bool					GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset = nullptr) const;
	bool					GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const;

	// Physics-based queries
	bool					GetFirstIntersectedEntity(const line3& Ray, vector3* pOutPoint3D = nullptr, CStrID* pOutEntityUID = nullptr) const;
	UPTR					GetEntitiesInPhysBox(CArray<CEntity*>& Out, const matrix44& OBB) const;
	UPTR					GetEntitiesInPhysSphere(CArray<CEntity*>& Out, const vector3& Center, float Radius) const;
	bool					GetSurfaceInfoBelow(CSurfaceInfo& Out, const vector3& Position, float ProbeLength = 1000.f) const;

	CStrID					GetID() const { return ID; }
	const CString&			GetName() const { return Name; }

	Scene::CSceneNode*		GetSceneRoot() { return SceneRoot.Get(); }
	Scene::CSPS&			GetSPS() { return SPS; }
	Physics::CPhysicsLevel*	GetPhysics() const { return PhysicsLevel.Get(); }
	AI::CAILevel*			GetAI() const { return AILevel.Get(); }
};

}
