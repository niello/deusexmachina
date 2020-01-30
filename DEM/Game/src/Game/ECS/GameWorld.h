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

template<class T>
struct ensure_pointer
{
	using type = std::remove_reference_t<std::remove_pointer_t<T>>*;
};

template<class T> using ensure_pointer_t = typename ensure_pointer<T>::type;

template<class T>
struct just_type
{
	using type = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;
};

template<class T> using just_type_t = typename just_type<T>::type;

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

	// Zero-based type index for fast component storage access
	inline static uint32_t ComponentTypeCount = 0;
	template<class T> inline static const uint32_t ComponentTypeIndex = ComponentTypeCount++;

	Resources::CResourceManager&   _ResMgr;

	CEntityStorage                 _Entities; //???add unordered_map index by name?
	std::vector<PComponentStorage> _Components;

	// system by type list
	// fast-access map entity -> components? Component stores entity ID, but need also to find components by entity ID and type

	// loaded level list (main place for them)
	//???accumulated COIs for levels?

	template<typename TComponent, typename... Components>
	bool GetNextComponents(HEntity EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out);

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

	//???
	// LoadEntityTemplate(desc)
	// CreateEntity(desc)

	HEntity     CreateEntity(/*level or level ID?*/) { return _Entities.Allocate(); }
	// CreateEntity(template)
	// CreateEntity(component type list, templated?)
	// CreateEntity(prototype entity ID for cloning)
	// DeleteEntity
	// MoveEntity(id, level[id?])
	bool        EntityExists(HEntity EntityID) const { return !!_Entities.GetValue(EntityID); }
	auto        GetEntity(HEntity EntityID) const { return _Entities.GetValue(EntityID); }
	auto        GetEntityUnsafe(HEntity EntityID) const { return _Entities.GetValueUnsafe(EntityID); }
	bool        IsEntityActive(HEntity EntityID) const { auto pEntity = _Entities.GetValue(EntityID); return pEntity && pEntity->IsActive; }
	CGameLevel* GetEntityLevel(HEntity EntityID) const { auto pEntity = _Entities.GetValue(EntityID); return pEntity ? pEntity->Level.Get() : nullptr; }

	template<class T> void RegisterComponent(UPTR InitialCapacity = 0);
	template<class T> T*   AddComponent(HEntity EntityID);
	template<class T> bool RemoveComponent(HEntity EntityID);
	template<class T> T*   FindComponent(HEntity EntityID);
	template<class T> typename TComponentTraits<T>::TStorage* FindComponentStorage();

	template<typename TComponent, typename... Components>
	void ForEachEntityWith(std::function<void(CEntity&, TComponent&&, ensure_pointer_t<Components>&&...)>&& Callback);

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
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Components.size() <= TypeIndex) _Components.resize(TypeIndex + 1);
	_Components[TypeIndex] = std::make_unique<TComponentTraits<T>::TStorage>(InitialCapacity);
}
//---------------------------------------------------------------------

template<class T>
T* CGameWorld::AddComponent(HEntity EntityID)
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::AddComponent() > T must be derived from CComponent");

	// Invalid entity ID is forbidden
	if (!EntityID) return nullptr;

	// Component type is not registered yet
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Components.size() <= TypeIndex || !_Components[TypeIndex]) return nullptr;

	//???check entity exists? create if not? or fail?

	auto& Storage = *static_cast<TComponentTraits<T>::TStorage*>(_Components[TypeIndex].get());
	return Storage.Add(EntityID);
}
//---------------------------------------------------------------------

template<class T>
bool CGameWorld::RemoveComponent(HEntity EntityID)
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::RemoveComponent() > T must be derived from CComponent");

	// Invalid entity ID is forbidden
	if (!EntityID) FAIL;

	// Component type is not registered yet
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Components.size() <= TypeIndex || !_Components[TypeIndex]) FAIL;

	//???check entity exists?

	auto& Storage = *static_cast<TComponentTraits<T>::TStorage*>(_Components[TypeIndex].get());
	return Storage.Remove(EntityID);
}
//---------------------------------------------------------------------

template<class T>
T* CGameWorld::FindComponent(HEntity EntityID)
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::FindComponent() > T must be derived from CComponent");

	// Invalid entity ID is forbidden
	if (!EntityID) return nullptr;

	// Component type is not registered yet
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Components.size() <= TypeIndex || !_Components[TypeIndex]) return nullptr;

	auto& Storage = *static_cast<TComponentTraits<T>::TStorage*>(_Components[TypeIndex].get());
	return Storage.Find(EntityID);
}
//---------------------------------------------------------------------

template<class T>
typename TComponentTraits<T>::TStorage* CGameWorld::FindComponentStorage()
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::FindComponentStorage() > T must be derived from CComponent");

	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Components.size() <= TypeIndex) return nullptr;

	return static_cast<TComponentTraits<T>::TStorage*>(_Components[TypeIndex].get());
}
//---------------------------------------------------------------------

// Used by join-iterator ForEachEntityWith()
template<typename TComponent, typename... Components>
bool CGameWorld::GetNextComponents(HEntity EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out)
{
	ensure_pointer_t<TComponent> pComponent = nullptr;
	std::tuple<ensure_pointer_t<Components>...> NextComponents;

	bool NextOk = true;
	if constexpr(sizeof...(Components) > 0)
		NextOk = GetNextComponents<Components...>(EntityID, NextComponents);

	if (NextOk)
		if (auto pStorage = FindComponentStorage<just_type_t<TComponent>>())
			pComponent = pStorage->Find(EntityID);

	Out = std::tuple_cat(std::make_tuple(pComponent), NextComponents);

	if constexpr (std::is_pointer_v<TComponent>)
		return true;
	else
		return !!pComponent;
}
//---------------------------------------------------------------------

// Join-iterator over entities containing a set of components. Components specified by pointer are optional.
// They are nullptr if not present. It is recommended to specify mandatory ones first. Respects 'const' specifier.
// Callback args are an entity ref followed by component pointers in the same order as in args.
// TODO: pass mandatory components by reference into a Callback?
// TODO: find storages once before the loop and save in a tuple!
template<typename TComponent, typename... Components>
void CGameWorld::ForEachEntityWith(std::function<void(CEntity&, TComponent&&, ensure_pointer_t<Components>&&...)>&& Callback)
{
	static_assert(!std::is_pointer_v<TComponent>, "First component in ForEachEntityWith must be mandatory!");

	// The first component is mandatory
	if (auto pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		for (auto&& Component : *pStorage)
		{
			auto pEntity = GetEntityUnsafe(Component.EntityID);
			if (!pEntity || !pEntity->IsActive) continue;

			std::tuple<ensure_pointer_t<Components>...> NextComponents;
			if (GetNextComponents<Components...>(Component.EntityID, NextComponents))
				std::apply(std::forward<decltype(Callback)>(Callback), std::tuple_cat(std::make_tuple(*pEntity, Component), NextComponents));
		}
	}
}
//---------------------------------------------------------------------

}
