#pragma once
#include <Game/ECS/Component.h>
#include <Math/AABB.h>
#include <typeindex>

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

class ISaveLoadDelegate
{
public:

	virtual ~ISaveLoadDelegate() = default;

	//???pass level or level ID?
	virtual bool LoadEntities(CGameWorld& World, CGameLevel& Level, const Data::CParams& ) = 0;
};

class CGameWorld final
{
protected:

	Resources::CResourceManager&                           _ResMgr;

	CEntityStorage                                         _Entities; //???add unordered_map index by name?
	std::unordered_map<std::type_index, PComponentStorage> _Components;

	// system by type list
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

	HEntity CreateEntity(/*level or level ID?*/) { return _Entities.Allocate(); }
	// CreateEntity(template)
	// CreateEntity(component type list, templated?)
	// CreateEntity(prototype entity ID for cloning)
	// DeleteEntity
	// EntityExists
	// MoveEntity(id, level[id?])

	//???
	// LoadEntityTemplate(desc)
	// CreateEntity(desc)

	template<class T> void RegisterComponent(UPTR InitialCapacity = 0);
	template<class T> T*   AddComponent(HEntity EntityID);
	template<class T> bool RemoveComponent(HEntity EntityID);
	// FindComponent<T>(entity)
	//???public iterators over components of requested type? return some wrapper with .begin() and .end()?

	// RegisterSystem<T>(system instance, update priority? or system tells its dependencies and world sorts them? explicit order?)
	// UnregisterSystem<T>()
	// GetSystem<T>()
};

template<class T>
void CGameWorld::RegisterComponent(UPTR InitialCapacity)
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::RegisterComponent() > T must be derived from CComponent");
	static_assert(std::is_base_of_v<IComponentStorage, TComponentTraits<T>::TStorage>,
		"CGameWorld::RegisterComponent() > Storage must implement IComponentStorage");

	// Static type is enough for distinguishing between different components, no dynamic RTTI needed
	auto Key = std::type_index(typeid(T));

	auto It = _Components.find(Key);
	if (It != _Components.cend()) return;

	_Components.emplace(Key, std::make_unique<TComponentTraits<T>::TStorage>(InitialCapacity));
}
//---------------------------------------------------------------------

template<class T>
T* CGameWorld::AddComponent(HEntity EntityID)
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::AddComponent() > T must be derived from CComponent");

	if (!EntityID) return nullptr;

	auto It = _Components.find(std::type_index(typeid(T)));
	if (It == _Components.cend()) return nullptr;

	//???check entity exists? create if not? or fail?

	auto& Storage = *static_cast<TComponentTraits<T>::TStorage*>(It->second.get());
	return Storage.Add(EntityID);
}
//---------------------------------------------------------------------

template<class T>
bool CGameWorld::RemoveComponent(HEntity EntityID)
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::AddComponent() > T must be derived from CComponent");

	if (!EntityID) FAIL;

	auto It = _Components.find(std::type_index(typeid(T)));
	if (It == _Components.cend()) return nullptr;

	//???check entity exists?

	auto& Storage = *static_cast<TComponentTraits<T>::TStorage*>(It->second.get());
	return Storage.Remove(EntityID);
}
//---------------------------------------------------------------------

}
