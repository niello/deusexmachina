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

class CGameWorld final
{
protected:

	// Zero-based type index for fast component storage access
	inline static uint32_t ComponentTypeCount = 0;
	template<class T> inline static const uint32_t ComponentTypeIndex = ComponentTypeCount++;

	Resources::CResourceManager&   _ResMgr;

	CEntityStorage                 _Entities; //???add unordered_map index by name?
	std::vector<PComponentStorage> _Storages;
	std::unordered_map<CStrID, IComponentStorage*> _StorageMap;

	// system by type list
	// fast-access map entity -> components? Component stores entity ID, but need also to find components by entity ID and type

	std::unordered_map<CStrID, PGameLevel> _Levels;
	//???accumulated COIs for levels?

	template<typename TComponent, typename... Components>
	bool GetNextStorages(std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Out);
	template<typename TComponent, typename... Components>
	bool GetNextComponents(HEntity EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out, const std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Storages);

public:

	CGameWorld(Resources::CResourceManager& ResMgr);

	void SaveEntities(CStrID LevelID, Data::CParams& Out) const;
	void LoadEntities(CStrID LevelID, const Data::CParams& In);
	void SaveEntitiesDiff(CStrID LevelID, Data::CParams& Out, const CGameWorld& Base) const;
	void LoadEntitiesDiff(CStrID LevelID, const Data::CParams& In);
	void SaveEntities(CStrID LevelID, IO::CBinaryWriter& Out) const;
	void LoadEntities(CStrID LevelID, IO::CBinaryReader& In);
	void SaveEntitiesDiff(CStrID LevelID, IO::CBinaryWriter& Out, const CGameWorld& Base) const;
	void LoadEntitiesDiff(CStrID LevelID, IO::CBinaryReader& In);

	// Update(float dt)

	// SetTimeFactor, <= 0 - pause
	// GetTimeFactor
	// GetTime

	CGameLevel* CreateLevel(CStrID ID, const CAABB& Bounds, const CAABB& InteractiveBounds = CAABB::Empty, UPTR SubdivisionDepth = 0);
	CGameLevel* LoadLevel(CStrID ID, const Data::CParams& In);
	CGameLevel* FindLevel(CStrID ID) const;
	// SaveLevel(id, out params / delegate)
	// UnloadLevel(id)
	// SetLevelActive(bool) - exclude level from updating, maybe even allow to unload its heavy resources?
	// ValidateLevels() - need API? or automatic? resmgr as param or stored inside the world? must be consistent across validations!
	// AddLevelCOI(id, vector3) - cleared after update

	//???
	// LoadEntityTemplate(desc)
	// CreateEntity(desc)

	HEntity     CreateEntity(CStrID LevelID);
	// CreateEntity(template)
	// CreateEntity(component type list, templated?)
	// CreateEntity(prototype entity ID for cloning)
	void        DeleteEntity(HEntity EntityID);
	// MoveEntity(id, level[id?])
	bool        EntityExists(HEntity EntityID) const { return !!_Entities.GetValue(EntityID); }
	auto        GetEntity(HEntity EntityID) const { return _Entities.GetValue(EntityID); }
	auto        GetEntityUnsafe(HEntity EntityID) const { return _Entities.GetValueUnsafe(EntityID); }
	const auto& GetEntities() const { return _Entities; }
	bool        IsEntityActive(HEntity EntityID) const { auto pEntity = _Entities.GetValue(EntityID); return pEntity && pEntity->IsActive; }
	CGameLevel* GetEntityLevel(HEntity EntityID) const { auto pEntity = _Entities.GetValue(EntityID); return pEntity ? pEntity->Level.Get() : nullptr; }

	template<class T> void  RegisterComponent(CStrID Name, UPTR InitialCapacity = 0);
	template<class T> T*    AddComponent(HEntity EntityID);
	template<class T> bool  RemoveComponent(HEntity EntityID);
	template<class T> T*    FindComponent(HEntity EntityID);
	template<class T> typename TComponentStoragePtr<T> FindComponentStorage();
	const IComponentStorage* FindComponentStorage(CStrID ComponentName) const;
	IComponentStorage* FindComponentStorage(CStrID ComponentName);

	template<typename TComponent, typename... Components, typename TCallback>
	void ForEachEntityWith(TCallback Callback);

	// RegisterSystem<T>(system instance, update priority? or system tells its dependencies and world sorts them? explicit order?)
	// UnregisterSystem<T>()
	// GetSystem<T>()
};

template<class T>
void CGameWorld::RegisterComponent(CStrID Name, UPTR InitialCapacity)
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::RegisterComponent() > T must be derived from CComponent");
	static_assert(std::is_base_of_v<IComponentStorage, TComponentTraits<T>::TStorage>,
		"CGameWorld::RegisterComponent() > Storage must implement IComponentStorage");

	// Static type is enough for distinguishing between different components, no dynamic RTTI needed
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex) _Storages.resize(TypeIndex + 1);
	_Storages[TypeIndex] = std::make_unique<TComponentTraits<T>::TStorage>(InitialCapacity);
	_StorageMap[Name] = _Storages[TypeIndex].get();
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
	if (_Storages.size() <= TypeIndex || !_Storages[TypeIndex]) return nullptr;

	//???check entity exists? create if not? or fail?

	auto& Storage = *static_cast<TComponentStoragePtr<T>>(_Storages[TypeIndex].get());
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
	if (_Storages.size() <= TypeIndex || !_Storages[TypeIndex]) FAIL;

	//???check entity exists?

	auto& Storage = *static_cast<TComponentStoragePtr<T>>(_Storages[TypeIndex].get());
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
	if (_Storages.size() <= TypeIndex || !_Storages[TypeIndex]) return nullptr;

	auto& Storage = *static_cast<TComponentStoragePtr<T>>(_Storages[TypeIndex].get());
	return Storage.Find(EntityID);
}
//---------------------------------------------------------------------

template<class T>
typename TComponentStoragePtr<T> CGameWorld::FindComponentStorage()
{
	static_assert(std::is_base_of_v<CComponent, T> && !std::is_same_v<CComponent, T>,
		"CGameWorld::FindComponentStorage() > T must be derived from CComponent");

	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex) return nullptr;

	return static_cast<TComponentStoragePtr<T>>(_Storages[TypeIndex].get());
}
//---------------------------------------------------------------------

template<typename TComponent, typename... Components>
inline bool CGameWorld::GetNextStorages(std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Out)
{
	TComponentStoragePtr<TComponent> pStorage = nullptr;
	std::tuple<TComponentStoragePtr<Components>...> NextStorages;

	bool NextOk = true;
	if constexpr(sizeof...(Components) > 0)
		NextOk = GetNextStorages<Components...>(NextStorages);

	if (NextOk)
		pStorage = FindComponentStorage<just_type_t<TComponent>>();

	Out = std::tuple_cat(std::make_tuple(pStorage), NextStorages);

	if constexpr (std::is_pointer_v<TComponent>)
		return true;
	else
		return !!pStorage;
}
//---------------------------------------------------------------------

// Used by join-iterator ForEachEntityWith()
template<typename TComponent, typename... Components>
DEM_FORCE_INLINE bool CGameWorld::GetNextComponents(HEntity EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out, const std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Storages)
{
	ensure_pointer_t<TComponent> pComponent = nullptr;
	std::tuple<ensure_pointer_t<Components>...> NextComponents;

	bool NextOk = true;
	if constexpr(sizeof...(Components) > 0)
		NextOk = GetNextComponents<Components...>(EntityID, NextComponents, tuple_pop_front(Storages));

	if (NextOk)
		if (auto pStorage = std::get<0>(Storages))
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
template<typename TComponent, typename... Components, typename TCallback>
inline void CGameWorld::ForEachEntityWith(TCallback Callback)
{
	static_assert(!std::is_pointer_v<TComponent>, "First component in ForEachEntityWith must be mandatory!");

	// The first component is mandatory
	if (auto pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		std::tuple<TComponentStoragePtr<Components>...> NextStorages;
		if constexpr(sizeof...(Components) > 0)
			if (!GetNextStorages<Components...>(NextStorages)) return;

		for (auto&& Component : *pStorage)
		{
			auto pEntity = GetEntityUnsafe(Component.EntityID);
			if (!pEntity || !pEntity->IsActive) continue;

			std::tuple<ensure_pointer_t<Components>...> NextComponents;
			if constexpr(sizeof...(Components) > 0)
				if (!GetNextComponents<Components...>(Component.EntityID, NextComponents, NextStorages)) continue;

			if constexpr (std::is_const_v<TComponent>)
				std::apply(std::forward<TCallback>(Callback), std::tuple_cat(std::make_tuple(*pEntity, std::cref(Component)), NextComponents));
			else
				std::apply(std::forward<TCallback>(Callback), std::tuple_cat(std::make_tuple(*pEntity, std::ref(Component)), NextComponents));
		}
	}
}
//---------------------------------------------------------------------

}
