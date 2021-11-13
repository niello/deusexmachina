#pragma once
#include <Core/RTTIBaseClass.h>
#include <Game/ECS/ComponentStorage.h>
#include <Game/ECS/EntityTemplate.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Math/AABB.h>
#include <sol/sol.hpp>

// A complete game world with objects, time and space. Space is subdivided into levels.
// All levels share the same time. Objects (or entities) can move between levels, but
// each object can exist only in one level at the same time. Gameplay systems are also
// registered here. Designed using an ECS (entity-component-system) pattern.
// Multiple isolated worlds can be created, but one is enough for any typical game.

namespace DEM::Game
{
typedef std::unique_ptr<class CGameWorld> PGameWorld;
typedef Ptr<class CGameLevel> PGameLevel;

// CRTTIBaseClass for registration in a CGameSession.
class CGameWorld final : public ::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(CGameWorld, ::Core::CRTTIBaseClass);

protected:

	// Zero-based type index for fast component storage access
	inline static uint32_t ComponentTypeCount = 0;
	template<typename T> inline static const uint32_t ComponentTypeIndex = ComponentTypeCount++;

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

	std::unordered_map<CStrID, PGameLevel> _Levels;
	//???accumulated COIs for levels?

	void LoadEntityFromParams(const Data::CParam& In, bool Diff);
	bool SaveEntityToParams(Data::CParams& Out, HEntity EntityID, const CEntity& Entity, const CEntity* pBaseEntity) const;
	bool InstantiateTemplate(HEntity EntityID, CStrID TemplateID, bool BaseState, bool Validate);

	template<typename TComponent, typename... Components>
	bool GetNextStorages(std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Out);
	template<typename TComponent, typename... Components>
	bool GetNextComponents(HEntity EntityID, std::tuple<ensure_pointer_t<TComponent>, ensure_pointer_t<Components>...>& Out, const std::tuple<TComponentStoragePtr<TComponent>, TComponentStoragePtr<Components>...>& Storages);

	template <typename TDest, typename TSrc>
	constexpr decltype(auto) RestoreComponentType(TSrc&& Src)
	{
		if constexpr (std::is_pointer_v<TDest>) return std::forward<TSrc>(Src);
		else return std::reference_wrapper<TDest>(*std::forward<TSrc>(Src));
	}
	//---------------------------------------------------------------------

	template <typename... Components, class TCallback, typename TComponent, typename ComponentTuple, size_t... I>
	constexpr decltype(auto) InvokeQueryCallback(TCallback&& _Obj, HEntity EntityID, const CEntity& Entity, TComponent&& MainComponent, ComponentTuple&& NextComponents, std::index_sequence<I...>)
	{
		return std::invoke(std::forward<TCallback>(_Obj), EntityID, Entity, std::forward<TComponent>(MainComponent),
			RestoreComponentType<std::tuple_element_t<I, std::tuple<Components...>>>(std::get<I>(std::forward<ComponentTuple>(NextComponents)))...);
	}
	//---------------------------------------------------------------------

public:

	CGameWorld(Resources::CResourceManager& ResMgr);

	void Start();
	void Stop();
	void FinalizeLoading();

	void ClearAll(UPTR NewInitialCapacity = 0);
	void ClearDiff();
	void LoadBase(const Data::CParams& In);
	void LoadBase(IO::PStream InStream);
	void LoadDiff(const Data::CParams& In);
	void LoadDiff(IO::PStream InStream);
	bool SaveAll(Data::CParams& Out);
	bool SaveAll(IO::CBinaryWriter& Out);
	bool SaveDiff(Data::CParams& Out);
	bool SaveDiff(IO::CBinaryWriter& Out);

	IO::IStream* GetBaseStream(U64 Offset) const;

	CGameLevel* CreateLevel(CStrID ID, const CAABB& Bounds, const CAABB& InteractiveBounds = CAABB::Empty, UPTR SubdivisionDepth = 0);
	CGameLevel* LoadLevel(CStrID ID, const Data::CParams& In);
	CGameLevel* FindLevel(CStrID ID) const;
	void        ValidateComponents(CStrID LevelID);
	void        InvalidateComponents(CStrID LevelID);
	// SaveLevel(id, out params / delegate)
	// UnloadLevel(id)
	// SetLevelActive(bool) - exclude level from updating, maybe even allow to unload its heavy resources?
	// ValidateLevels() - need API? or automatic? resmgr as param or stored inside the world? must be consistent across validations!
	// AddLevelCOI(id, vector3) - cleared after update

	HEntity        CreateEntity(CStrID LevelID, CStrID TemplateID = CStrID::Empty);
	// CreateEntity(template ID)
	// CreateEntity(component type list, templated?)
	// CreateEntity(prototype entity ID for cloning)
	void           DeleteEntity(HEntity EntityID);
	// MoveEntity(id, level[id?])
	bool           EntityExists(HEntity EntityID) const { return !!_Entities.GetValue(EntityID); }
	const CEntity* GetEntity(HEntity EntityID) const { return _Entities.GetValue(EntityID); }
	const CEntity& GetEntityUnsafe(HEntity EntityID) const { return _Entities.GetValueUnsafe(EntityID); }
	const auto&    GetEntities() const { return _Entities; }
	bool           IsEntityActive(HEntity EntityID) const { auto pEntity = _Entities.GetValue(EntityID); return pEntity && pEntity->IsActive; }
	CStrID         GetEntityLevel(HEntity EntityID) const { auto pEntity = _Entities.GetValue(EntityID); return pEntity ? pEntity->LevelID : CStrID::Empty; }

	template<class T> void        RegisterComponent(CStrID Name, UPTR InitialCapacity = 0);
	template<class T> T*          AddComponent(HEntity EntityID);
	template<class T> bool        RemoveComponent(HEntity EntityID);
	template<class T> void        RemoveAllComponents();
	template<class T> size_t      GetComponentCount() const;
	template<class T> T*          FindComponent(HEntity EntityID) const;
	template<class T>
	TComponentStoragePtr<T>       FindComponentStorage();
	template<class T>
	const TComponentStoragePtr<T> FindComponentStorage() const;
	const IComponentStorage*      FindComponentStorage(CStrID ComponentName) const;
	IComponentStorage*            FindComponentStorage(CStrID ComponentName);

	template<class T>
	const Data::CData*            GetTemplateComponentData(CStrID TemplateID) const;
	template<class T>
	const Data::CData*            GetTemplateComponentData(HEntity EntityID) const;

	template<typename TComponent, typename... Components, typename TCallback>
	void ForEachEntityWith(TCallback Callback);
	template<typename TComponent, typename... Components, typename TCallback>
	void ForEachEntityInLevelWith(CStrID LevelID, TCallback Callback);
	template<typename TComponent, typename... Components, typename TCallback, typename TFilter>
	void ForEachEntityWith(TCallback Callback, TFilter Filter);
	template<typename TComponent, typename TCallback>
	void ForEachComponent(TCallback Callback);
	template<typename TComponent, typename TCallback>
	void FreeDead(TCallback DeinitCallback);

	//???TMP?
	//???create world based on session (constructor arg) and init fields table in a constructor?
	sol::table _ScriptFields;
	void InitScript(sol::state& Lua)
	{
		if (!_ScriptFields.valid()) _ScriptFields = Lua.create_table();
	}
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

	// If scripting is enabled, add component getter for scripts
	if (_ScriptFields.valid())
	{
		_ScriptFields[Name.CStr()] = [pStorage = _Storages[TypeIndex].get()](HEntity EntityID)
		{
			return static_cast<TComponentTraits<T>::TStorage*>(pStorage)->Find(EntityID);
		};
	}
}
//---------------------------------------------------------------------

template<class T>
T* CGameWorld::AddComponent(HEntity EntityID)
{
	if (!EntityID || !_Entities.GetValue(EntityID)) return nullptr;
	auto pStorage = FindComponentStorage<T>();
	return pStorage ? pStorage->Add(EntityID) : nullptr;
}
//---------------------------------------------------------------------

template<class T>
bool CGameWorld::RemoveComponent(HEntity EntityID)
{
	if (!EntityID) return false;
	auto pStorage = FindComponentStorage<T>();
	return pStorage ? pStorage->TComponentTraits<T>::TStorage::RemoveComponent(EntityID) : false;
}
//---------------------------------------------------------------------

template<class T>
void CGameWorld::RemoveAllComponents()
{
	if (auto pStorage = FindComponentStorage<T>())
		pStorage->TComponentTraits<T>::TStorage::ClearAll();
}
//---------------------------------------------------------------------

template<class T>
size_t CGameWorld::GetComponentCount() const
{
	auto pStorage = FindComponentStorage<T>();
	return pStorage ? pStorage->GetComponentCount() : 0;
}
//---------------------------------------------------------------------

template<class T>
T* CGameWorld::FindComponent(HEntity EntityID) const
{
	if (!EntityID) return nullptr;

	// NB: explicit storage type is important here because access to the const storage is optimized
	TComponentStoragePtr<T> pStorage = FindComponentStorage<just_type_t<T>>();
	return pStorage ? pStorage->Find(EntityID) : nullptr;
}
//---------------------------------------------------------------------

template<class T>
const typename TComponentStoragePtr<T> CGameWorld::FindComponentStorage() const
{
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex) return nullptr;
	auto pStorage = _Storages[TypeIndex].get();
	return pStorage ? static_cast<TComponentStoragePtr<T>>(pStorage) : nullptr;
}
//---------------------------------------------------------------------

template<class T>
typename TComponentStoragePtr<T> CGameWorld::FindComponentStorage()
{
	const auto TypeIndex = ComponentTypeIndex<T>;
	if (_Storages.size() <= TypeIndex) return nullptr;
	auto pStorage = _Storages[TypeIndex].get();
	return pStorage ? static_cast<TComponentStoragePtr<T>>(pStorage) : nullptr;
}
//---------------------------------------------------------------------

template<class T>
const Data::CData* CGameWorld::GetTemplateComponentData(CStrID TemplateID) const
{
	if (!TemplateID || !FindComponentStorage<T>()) return nullptr;

	auto pRsrc = _ResMgr.RegisterResource<CEntityTemplate>(TemplateID.CStr());
	if (!pRsrc) return nullptr;

	auto pTpl = pRsrc->ValidateObject<CEntityTemplate>();
	if (!pTpl) return nullptr;

	Data::CData* pData;
	return pTpl->GetDesc().TryGet(pData, _StorageIDs[ComponentTypeIndex<T>]) ? pData : nullptr;
}
//---------------------------------------------------------------------

template<class T>
const Data::CData* CGameWorld::GetTemplateComponentData(HEntity EntityID) const
{
	const CEntity* pEntity = GetEntity(EntityID);
	return pEntity ? GetTemplateComponentData<T>(pEntity->TemplateID) : nullptr;
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

// Join-iterator over active entities containing a set of components. See ForEachEntityWith(Callback, Filter).
// Callback args: entity ID, entity const ref, component pointers or refs in the same order and constness as in tpl.
template<typename TComponent, typename... Components, typename TCallback>
inline void CGameWorld::ForEachEntityWith(TCallback Callback)
{
	ForEachEntityWith<TComponent, Components...>(std::forward<TCallback>(Callback), [](HEntity EntityID, const CEntity& Entity)
	{
		return Entity.IsActive;
	});
}
//---------------------------------------------------------------------

// Join-iterator over active entities in a certain level with a set of components. See ForEachEntityWith(Callback, Filter).
// Callback args: entity ID, entity const ref, component pointers or refs in the same order and constness as in tpl.
template<typename TComponent, typename... Components, typename TCallback>
inline void CGameWorld::ForEachEntityInLevelWith(CStrID LevelID, TCallback Callback)
{
	ForEachEntityWith<TComponent, Components...>(std::forward<TCallback>(Callback), [LevelID](HEntity EntityID, const CEntity& Entity)
	{
		return Entity.IsActive && Entity.LevelID == LevelID;
	});
}
//---------------------------------------------------------------------

// Join-iterator over entities containing a set of components. Components specified by pointer are optional.
// They are nullptr if not present. It is recommended to specify mandatory ones first. Respects 'const' specifier.
// Callback args: entity ID, entity const ref, component pointers or refs in the same order and constness as in tpl.
template<typename TComponent, typename... Components, typename TCallback, typename TFilter>
inline void CGameWorld::ForEachEntityWith(TCallback Callback, TFilter Filter)
{
	static_assert(!std::is_pointer_v<TComponent>, "First component in ForEachEntityWith must be mandatory!");

	// The first component is mandatory
	// NB: explicit storage type is important here because access to the const storage is optimized
	if (TComponentStoragePtr<TComponent> pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		// Collect a tuple of requested component storages once outside the main loop
		std::tuple<TComponentStoragePtr<Components>...> NextStorages; (void)NextStorages;
		if constexpr(sizeof...(Components) > 0)
			if (!GetNextStorages<Components...>(NextStorages)) return;

		// Iterate the first, mandatory component
		// NB: choose that of your mandatory components that will most probably have the least instance count
		// TODO: can split into slices and dispatch to different threads (ForEachEntityWith<...>(Start, Count)?)
		for (auto&& [Component, EntityID] : *pStorage)
		{
			auto&& Entity = GetEntityUnsafe(EntityID);
			if (!Filter(EntityID, Entity)) continue;

			std::tuple<ensure_pointer_t<Components>...> NextComponents;
			if constexpr(sizeof...(Components) > 0)
				if (!GetNextComponents<Components...>(EntityID, NextComponents, NextStorages)) continue;

			InvokeQueryCallback<Components...>(std::forward<TCallback>(Callback), EntityID, Entity, std::reference_wrapper<TComponent>(Component), NextComponents, std::index_sequence_for<Components...>{});
		}
	}
}
//---------------------------------------------------------------------

// Callback args: entity ID, component [const] ref
template<typename TComponent, typename TCallback>
inline void CGameWorld::ForEachComponent(TCallback Callback)
{
	// NB: explicit storage type is important here because access to the const storage is optimized
	if (TComponentStoragePtr<TComponent> pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		// TODO: can split into slices and dispatch to different threads (ForEachComponent<...>(Start, Count)?)
		for (auto&& [Component, EntityID] : *pStorage)
		{
			// Prevent accessing mutable reference in callback if read-only component is requested
			if constexpr (std::is_const_v<TComponent>)
				Callback(EntityID, std::cref(Component));
			else
				Callback(EntityID, std::ref(Component));
		}
	}
}
//---------------------------------------------------------------------

// Delayed deinitialization for components with ExternalDeinit-enabled storage
// Callback args: entity ID, component ref
template<typename TComponent, typename TCallback>
inline void CGameWorld::FreeDead(TCallback DeinitCallback)
{
	if (auto pStorage = FindComponentStorage<just_type_t<TComponent>>())
	{
		for (auto&& [Component, EntityID] : pStorage->_Dead)
			DeinitCallback(EntityID, std::ref(Component));

		pStorage->FreeAllDead();
	}
}
//---------------------------------------------------------------------

}
