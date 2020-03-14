#pragma once
#include <Game/ECS/ComponentStorage.h>
#include <Game/ECS/EntityTemplate.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Math/AABB.h>
#include <typeindex>

// A complete game world with objects, time and space. Space is subdivided into levels.
// All levels share the same time. Objects (or entities) can move between levels, but
// each object can exist only in one level at the same time. Gameplay systems are also
// registered here. Designed using an ECS (entity-component-system) pattern.
// Multiple isolated worlds can be created, but one is enough for any typical game.

// TODO: move entity management and API into separate object and store it inside? World.Entities().Create(...) etc.

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

	enum class EState
	{
		Stopped = 0, // World is ready (has actual state) but simulation is disabled
		BaseLoaded,  // Base data is loaded but not applied to an actual state
		Running      // World is running a simulation, its state is actual
	};

	EState                         _State = EState::Stopped;

	Resources::CResourceManager&   _ResMgr;
	IO::PStream                    _BaseStream; // Base data is accessed on demand in RAM or in a mapped file

	CEntityStorage                 _EntitiesBase;
	CEntityStorage                 _Entities;
	std::vector<PComponentStorage> _Storages;
	std::vector<CStrID>            _StorageIDs;
	std::unordered_map<CStrID, IComponentStorage*> _StorageMap;

	// system by type list
	// fast-access map entity -> components? Component stores entity ID, but need also to find components by entity ID and type

	std::unordered_map<CStrID, PGameLevel> _Levels;
	//???accumulated COIs for levels?

	bool InstantiateTemplate(HEntity EntityID, CStrID TemplateID);

	template<typename TComponent, typename... Components>
	bool GetNextStorages(std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Out);
	template<typename TComponent, typename... Components>
	bool GetNextComponents(HEntity EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out, const std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Storages);

public:

	CGameWorld(Resources::CResourceManager& ResMgr);

	void Start();
	void Stop();
	void FinalizeLoading();

	//void ClearAll();
	//void ClearDiff();
	void LoadBase(const Data::CParams& In);
	void LoadBase(IO::PStream InStream);
	void LoadDiff(const Data::CParams& In);
	void LoadDiff(IO::PStream InStream);
	bool SaveAll(Data::CParams& Out);
	bool SaveAll(IO::CBinaryWriter& Out);
	bool SaveDiff(Data::CParams& Out);
	bool SaveDiff(IO::CBinaryWriter& Out);

	IO::IStream* GetBaseStream(U64 Offset) const;

	// Update(float dt)

	// SetTimeFactor, <= 0 - pause
	// GetTimeFactor
	// GetTime

	CGameLevel* CreateLevel(CStrID ID, const CAABB& Bounds, const CAABB& InteractiveBounds = CAABB::Empty, UPTR SubdivisionDepth = 0);
	CGameLevel* LoadLevel(CStrID ID, const Data::CParams& In);
	CGameLevel* FindLevel(CStrID ID) const;
	void        ValidateLevel(CStrID LevelID);
	void        InvalidateLevel(CStrID LevelID);
	// SaveLevel(id, out params / delegate)
	// UnloadLevel(id)
	// SetLevelActive(bool) - exclude level from updating, maybe even allow to unload its heavy resources?
	// ValidateLevels() - need API? or automatic? resmgr as param or stored inside the world? must be consistent across validations!
	// AddLevelCOI(id, vector3) - cleared after update

	//???
	// LoadEntityTemplate(desc)
	// CreateEntity(desc)

	HEntity     CreateEntity(CStrID LevelID, CStrID TemplateID = CStrID::Empty);
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

	template<class T> bool  GetTemplateComponent(CStrID TemplateID, T& Out) const;
	template<class T> bool  HasTemplateComponent(CStrID TemplateID) const;
	template<class T> bool  HasTemplateComponent(HEntity EntityID) const;

	template<typename TComponent, typename... Components, typename TCallback>
	void ForEachEntityWith(TCallback Callback);

	// RegisterSystem<T>(system instance, update priority? or system tells its dependencies and world sorts them? explicit order?)
	// UnregisterSystem<T>()
	// GetSystem<T>()
};

template<class T>
void CGameWorld::RegisterComponent(CStrID Name, UPTR InitialCapacity)
{
	static_assert(std::is_base_of_v<IComponentStorage, TComponentTraits<T>::TStorage>,
		"CGameWorld::RegisterComponent() > Storage must implement IComponentStorage");

	// Static type is enough for distinguishing between different components, no dynamic RTTI needed
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex)
	{
		_Storages.resize(TypeIndex + 1);
		_StorageIDs.resize(TypeIndex + 1);
	}
	_Storages[TypeIndex] = std::make_unique<TComponentTraits<T>::TStorage>(*this, InitialCapacity);
	_StorageIDs[TypeIndex] = Name;
	_StorageMap[Name] = _Storages[TypeIndex].get();
}
//---------------------------------------------------------------------

template<class T>
T* CGameWorld::AddComponent(HEntity EntityID)
{
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
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex) return nullptr;

	return static_cast<TComponentStoragePtr<T>>(_Storages[TypeIndex].get());
}
//---------------------------------------------------------------------

template<class T>
bool CGameWorld::GetTemplateComponent(CStrID TemplateID, T& Out) const
{
	// Component type is not registered yet
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex || !_Storages[TypeIndex]) return false;

	auto pRsrc = _ResMgr.FindResource(TemplateID);
	if (!pRsrc) return false;

	auto pTpl = pRsrc->ValidateObject<CEntityTemplate>();
	if (!pTpl) return false;

	Data::CData* pData;
	if (!pTpl->GetDesc().TryGet(pData, _StorageIDs[TypeIndex])) return false;

	return static_cast<TComponentStoragePtr<T>>(_Storages[TypeIndex].get())->DeserializeComponentFromParams(Out, *pData);
}
//---------------------------------------------------------------------

template<class T>
bool CGameWorld::HasTemplateComponent(CStrID TemplateID) const
{
	// Component type is not registered yet
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex || !_Storages[TypeIndex]) return false;

	auto pRsrc = _ResMgr.FindResource(TemplateID);
	if (!pRsrc) return false;

	auto pTpl = pRsrc->ValidateObject<CEntityTemplate>();
	return pTpl && pTpl->GetDesc().Has(_StorageIDs[TypeIndex]);
}
//---------------------------------------------------------------------

template<class T>
bool CGameWorld::HasTemplateComponent(HEntity EntityID) const
{
	const CEntity* pEntity = GetEntity(EntityID);
	return pEntity && pEntity->TemplateID && HasTemplateComponent<T>(pEntity->TemplateID);
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
// Callback args: entity ID, entity ref, component pointers in the same order as in args.
// TODO: pass mandatory components by reference into a Callback?
template<typename TComponent, typename... Components, typename TCallback>
inline void CGameWorld::ForEachEntityWith(TCallback Callback)
{
	static_assert(!std::is_pointer_v<TComponent>, "First component in ForEachEntityWith must be mandatory!");

	// The first component is mandatory
	if (auto pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		// Collect a tuple of requested component storages once outside the main loop
		std::tuple<TComponentStoragePtr<Components>...> NextStorages;
		if constexpr(sizeof...(Components) > 0)
			if (!GetNextStorages<Components...>(NextStorages)) return;

		// Iterate the first, mandatory component
		// NB: choose that of your mandatory components that will most probably have the least instance count
		// TODO: can split into slices and dispatch to different threads (ForEachEntityWith<...>(Start, Count)?)
		for (auto&& [Component, EntityID] : *pStorage)
		{
			auto pEntity = GetEntityUnsafe(EntityID);
			if (!pEntity || !pEntity->IsActive) continue;

			std::tuple<ensure_pointer_t<Components>...> NextComponents;
			if constexpr(sizeof...(Components) > 0)
				if (!GetNextComponents<Components...>(EntityID, NextComponents, NextStorages)) continue;

			if constexpr (std::is_const_v<TComponent>)
				std::apply(std::forward<TCallback>(Callback), std::tuple_cat(std::make_tuple(EntityID, *pEntity, std::cref(Component)), NextComponents));
			else
				std::apply(std::forward<TCallback>(Callback), std::tuple_cat(std::make_tuple(EntityID, *pEntity, std::ref(Component)), NextComponents));
		}
	}
}
//---------------------------------------------------------------------

}
