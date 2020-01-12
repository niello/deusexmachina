#pragma once
#include <Events/EventDispatcher.h>
#include <Scene/SPS.h>
#include <Frame/GraphicsResourceManager.h> // FIXME: view, not model!
#include <Data/Regions.h>

// Represents one game location, including all entities in it and property worlds (physics, AI, scene).
// In MVC pattern level would be a Model.

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
	typedef Ptr<class CSceneNode> PSceneNode;
}

namespace Physics
{
	typedef std::unique_ptr<class CPhysicsLevel> PPhysicsLevel;
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

	// FIXME: view, not model!
	Frame::CGraphicsResourceManager* _pGRM = nullptr;

	CGameLevel();
	virtual ~CGameLevel();

	bool					Load(CStrID LevelID, const Data::CParams& Desc);
	bool					Validate(Frame::CGraphicsResourceManager* pGRM);
	void					Term();
	bool					Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc = nullptr);
	//void					RenderDebug();

	//???GetEntityAABB(AABB_Gfx | AABB_Phys);?

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

	//Other queries
	bool					HostsEntity(CStrID EntityID) const;

	CStrID					GetID() const { return ID; }
	const CString&			GetName() const { return Name; }

	Scene::CSceneNode*		GetSceneRoot() { return SceneRoot.Get(); }
	Scene::CSPS&			GetSPS() { return SPS; }
	Physics::CPhysicsLevel*	GetPhysics() const { return PhysicsLevel.get(); }
	AI::CAILevel*			GetAI() const { return AILevel.Get(); }
};

typedef Ptr<CGameLevel> PGameLevel;

}
