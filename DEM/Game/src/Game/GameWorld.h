#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Math/AABB.h>

// A complete game world with objects, time and space. Space is subdivided into levels.
// All levels share the same time. Objects (or entities) can move between levels, but
// each object can exist only in one level at the same time. Gameplay systems are also
// registered here. Designed using an ECS (entity-component-system) pattern.
// Multiple isolated worlds can be created, but one is enough for any typical game.

// TODO: move entity management and API into separate object and store it inside? World.Entities().Create(...) etc.

namespace Data
{
	class CParams;
}

namespace Resources
{
	class CResourceManager;
}

namespace DEM::Game
{
typedef std::unique_ptr<class CGameWorld> PGameWorld;
typedef Ptr<class CGameLevel> PGameLevel;
class ISaveLoadDelegate;

class CGameWorld final
{
protected:

	Resources::CResourceManager& _ResMgr;

	// entity by level list (or store in levels?), entity must know its current level (in some component? or component remembers if wants?)
	// components by type list
	// system by type list

	// component pools by type
	// fast-access map entity -> components? Component stores entity ID, but need also to find components by entity ID and type

	// loaded level list (main place for them)
	//???accumulated COIs for levels?

public:

	CGameWorld(Resources::CResourceManager& ResMgr);

	//???factory or n_new, then load in a client code?
	// LoadState(params / delegate)
	// SaveState(params / delegate)

	// Update(float dt)

	// SetTimeFactor, <= 0 - pause
	// GetTimeFactor
	// GetTime

	CGameLevel* CreateLevel(CStrID ID, const CAABB& Bounds, const CAABB& InteractiveBounds = CAABB::Empty, UPTR SubdivisionDepth = 0);
	CGameLevel* LoadLevel(CStrID ID, const Data::CParams& BaseData, ISaveLoadDelegate* pStateLoader = nullptr);
	// SaveLevel(id, out params / delegate)
	// UnloadLevel(id)
	// FindLevel(id)
	// SetLevelActive(bool) - exclude level from updating, maybe even allow to unload its heavy resources?
	// ValidateLevels() - need API? or automatic? resmgr as param or stored inside the world? must be consistent across validations!
	// AddLevelCOI(id, vector3) - cleared after update

	// CreateEntity(template)
	// CreateEntity(component type list)
	// CreateEntity(prototype entity ID for cloning)
	// DeleteEntity
	// EntityExists
	// MoveEntity(id, level[id?])

	//???
	// LoadEntityTemplate(desc)
	// CreateEntity(desc)

	// AddComponent<T>(entity)
	// RemoveComponent<T>(entity)
	// FindComponent<T>(entity)
	//???public iterators over components of requested type? return some wrapper with .begin() and .end()?

	// RegisterSystem<T>(system instance, update priority? or system tells its dependencies and world sorts them? explicit order?)
	// UnregisterSystem<T>()
	// GetSystem<T>()
};

}
