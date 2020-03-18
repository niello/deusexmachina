#include "GameWorld.h"
#include <Game/GameLevel.h>
#include <AI/AILevel.h>
#include <Scene/SceneNode.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <unordered_set>

namespace DEM::Game
{

CGameWorld::CGameWorld(Resources::CResourceManager& ResMgr)
	: _ResMgr(ResMgr)
{
}
//---------------------------------------------------------------------

// TODO: notify systems
void CGameWorld::Start()
{
	if (_State != EState::Running)
	{
		FinalizeLoading();
		_State = EState::Running;
	}
}
//---------------------------------------------------------------------

// TODO: notify systems
void CGameWorld::Stop()
{
	// Can't start or stop the world in a BaseLoaded state
	if (_State == EState::Running)
		_State = EState::Stopped;
}
//---------------------------------------------------------------------

void CGameWorld::FinalizeLoading()
{
	// Only a BaseLoaded state really requires finalization
	if (_State != EState::BaseLoaded) return;

	// The world is loaded without a diff. Copy base state into the actual one.
	for (const auto& BaseEntity : _EntitiesBase)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _EntitiesBase.GetHandle(&BaseEntity);

		const auto RealEntityID = _Entities.AllocateWithHandle(EntityID.Raw, BaseEntity);
		n_assert_dbg(RealEntityID && RealEntityID == EntityID);
	}

	_State = EState::Stopped;
}
//---------------------------------------------------------------------

void CGameWorld::ClearAll(UPTR NewInitialCapacity)
{
	// TODO: notify affected systems / entities about state destruction

	_BaseStream.Reset();
	_EntitiesBase.Clear(NewInitialCapacity);
	_Entities.Clear(NewInitialCapacity);
	for (auto& Storage : _Storages)
		Storage->ClearAll();
}
//---------------------------------------------------------------------

void CGameWorld::ClearDiff()
{
	if (_State == EState::BaseLoaded || _EntitiesBase.empty()) return;

	// TODO: notify affected systems / entities about state destruction

	_Entities.Clear(_EntitiesBase.size());
	for (auto& Storage : _Storages)
		Storage->ClearDiff();

	_State = EState::BaseLoaded;
}
//---------------------------------------------------------------------

void CGameWorld::LoadEntityFromParams(const Data::CParam& In, bool Diff)
{
	// Entity is always named as __UID. Skip leading "__".
	const auto RawHandle = static_cast<CEntityStorage::THandleValue>(std::atoi(In.GetName().CStr() + 2));

	HEntity EntityID{ RawHandle };
	CEntity* pEntity = Diff ? _Entities.GetValue(EntityID) : nullptr;
	if (!pEntity)
	{
		EntityID = _Entities.AllocateWithHandle(RawHandle, {});
		n_assert_dbg(EntityID);

		pEntity = _Entities.GetValueUnsafe(EntityID);
	}

	// Load entity diff or full data
	DEM::ParamsFormat::Deserialize(In.GetRawValue(), *pEntity);

	// Load component diffs or full data
	const Data::CParams& SEntity = *In.GetValue<Data::PParams>();
	for (const auto& ComponentParam : SEntity)
	{
		auto pStorage = FindComponentStorage(ComponentParam.GetName());
		if (!pStorage) continue;

		if (ComponentParam.GetRawValue().IsVoid())
			pStorage->RemoveComponent(EntityID);
		else
			pStorage->AddFromParams(EntityID, ComponentParam.GetRawValue());
	}

	// Load purely templated components
	InstantiateTemplate(EntityID, pEntity->TemplateID, !Diff, false);
}
//---------------------------------------------------------------------

// Saves full data or diff only, dependent on whether pBaseEntity is specified or not
bool CGameWorld::SaveEntityToParams(Data::CParams& Out, HEntity EntityID, const CEntity& Entity, const CEntity* pBaseEntity) const
{
	n_assert2(!pBaseEntity || Entity.TemplateID == pBaseEntity->TemplateID, "Entity template must never change at runtime!");

	Data::CData SEntity;
	DEM::ParamsFormat::SerializeDiff(SEntity, Entity, pBaseEntity ? *pBaseEntity : CEntity());

	for (const auto& [ComponentID, Storage] : _StorageMap)
	{
		Data::CData SComponent;

		const bool HasComponentInfo = pBaseEntity ?
			Storage->SaveComponentDiffToParams(EntityID, SComponent) :
			Storage->SaveComponentToParams(EntityID, SComponent);

		if (HasComponentInfo)
		{
			if (SEntity.IsVoid()) SEntity = Data::PParams(n_new(Data::CParams()));
			SEntity.GetValue<Data::PParams>()->Set(ComponentID, std::move(SComponent));
		}
	}

	// Diff saving is requested, and no difference detected
	if (pBaseEntity && SEntity.IsVoid()) return false;

	Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()),
		SEntity.IsVoid() ? Data::PParams(n_new(Data::CParams())) : std::move(SEntity.GetValue<Data::PParams>()));

	return true;
}
//---------------------------------------------------------------------

// Params loader doesn't support a separate base state. Use for editors and debug only.
// Diff will be populated with the whole world data.
void CGameWorld::LoadBase(const Data::CParams& In)
{
	// Erase all previous data in the world
	ClearAll(In.GetCount());

	// Unlike in the binary format, in params the world is stored per-entity
	for (const auto& EntityParam : In)
		LoadEntityFromParams(EntityParam, false);

	_State = EState::Stopped;
}
//---------------------------------------------------------------------

void CGameWorld::LoadBase(IO::PStream InStream)
{
	if (!InStream) return;

	IO::CBinaryReader In(*InStream);

	const auto EntityCount = In.Read<uint32_t>();

	// TODO: notify affected systems / entities about state destruction

	// Erase all previous data in the world
	_EntitiesBase.Clear(EntityCount);
	_Entities.Clear(EntityCount);

	// Required for delayed loading of components
	_BaseStream = InStream;

	// Load base list of entities
	for (uint32_t i = 0; i < EntityCount; ++i)
	{
		const auto EntityIDRaw = In.Read<CEntityStorage::THandleValue>();

		CEntity NewEntity;
		DEM::BinaryFormat::Deserialize(In, NewEntity);

		const auto EntityID = _EntitiesBase.AllocateWithHandle(EntityIDRaw, std::move(NewEntity));
		n_assert_dbg(EntityID && EntityID == HEntity{ EntityIDRaw });
	}

	// Load base state of components
	const auto ComponentTypeCount = In.Read<uint32_t>();
	for (uint32_t i = 0; i < ComponentTypeCount; ++i)
	{
		const auto TypeID = In.Read<CStrID>();
		if (auto pStorage = FindComponentStorage(TypeID))
			pStorage->LoadBase(In);
	}

	// Load purely templated components
	//???can delay to FinalizeLoading / LoadDiff? What about ClearDiff, where NoBase templates are deleted?
	for (const auto& Entity : _EntitiesBase)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);
		InstantiateTemplate(EntityID, Entity.TemplateID, true, false);
	}

	// The primary purpose of this state is an optimization. User either loads a diff,
	// which initializes an actual world state, or starts the world without the diff,
	// in which case the actual world state is delay-built as a copy of base state.
	// Without this state we would possibly redundantly initialize an actual state here.
	_State = EState::BaseLoaded;
}
//---------------------------------------------------------------------

void CGameWorld::LoadDiff(const Data::CParams& In)
{
	// Clear previous diff info, keep base intact
	ClearDiff();

	for (const auto& EntityParam : In)
	{
		if (EntityParam.GetRawValue().IsVoid())
		{
			// Entity is explicitly deleted. Remove it and all its components.
			// Entity is always named as __UID. Skip leading "__".
			const auto RawHandle = static_cast<CEntityStorage::THandleValue>(std::atoi(EntityParam.GetName().CStr() + 2));
			DeleteEntity({ RawHandle });
		}
		else
		{
			LoadEntityFromParams(EntityParam, true);
		}
	}

	_State = EState::Stopped;
}
//---------------------------------------------------------------------

void CGameWorld::LoadDiff(IO::PStream InStream)
{
	if (!InStream) return;

	// TODO: notify affected systems / entities about state destruction

	// Clear previous diff info, keep base intact
	if (_State != EState::BaseLoaded)
		_Entities.Clear(_EntitiesBase.size());

	IO::CBinaryReader In(*InStream);

	// Read deleted entity list
	std::unordered_set<HEntity> Deleted;
	Deleted.reserve(_EntitiesBase.size()); // Cant mark deleted more entities than in base

	HEntity EntityID = { In.Read<decltype(HEntity::Raw)>() };
	while (EntityID)
	{
		Deleted.insert(EntityID);
		EntityID = { In.Read<decltype(HEntity::Raw)>() };
	}

	// Read added / modified entity list
	EntityID = { In.Read<decltype(HEntity::Raw)>() };
	while (EntityID)
	{
		auto pBaseEntity = _EntitiesBase.GetValue(EntityID);
		CEntity NewEntity = pBaseEntity ? *pBaseEntity : CEntity{};
		DEM::BinaryFormat::DeserializeDiff(In, NewEntity);

		const auto NewEntityID = _Entities.AllocateWithHandle(EntityID, std::move(NewEntity));
		n_assert_dbg(NewEntityID && NewEntityID == EntityID);

		EntityID = { In.Read<decltype(HEntity::Raw)>() };
	}

	// Copy unchanged entities from the base state
	for (const auto& BaseEntity : _EntitiesBase)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto BaseEntityID = _EntitiesBase.GetHandle(&BaseEntity);
		if (!_Entities.GetValue(BaseEntityID) && Deleted.find(BaseEntityID) == Deleted.cend())
		{
			const auto EntityID = _Entities.AllocateWithHandle(BaseEntityID, BaseEntity);
			n_assert_dbg(EntityID && EntityID == BaseEntityID);
		}
	}

	// Load components
	const auto ComponentTypeCount = In.Read<uint32_t>();
	for (uint32_t i = 0; i < ComponentTypeCount; ++i)
	{
		const auto TypeID = In.Read<CStrID>();
		if (auto pStorage = FindComponentStorage(TypeID))
			pStorage->LoadDiff(In);
	}

	// Load purely templated components
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);
		InstantiateTemplate(EntityID, Entity.TemplateID, false, false);
	}

	_State = EState::Stopped;
}
//---------------------------------------------------------------------

bool CGameWorld::SaveAll(Data::CParams& Out)
{
	if (_State == EState::BaseLoaded) return false;

	// Unlike in the binary format, in params the world is stored per-entity
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);
		SaveEntityToParams(Out, EntityID, Entity, nullptr);
	}

	return true;
}
//---------------------------------------------------------------------

bool CGameWorld::SaveAll(IO::CBinaryWriter& Out)
{
	if (_State == EState::BaseLoaded) return false;

	Out.Write(static_cast<uint32_t>(_Entities.size()));
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		//???save only index part in SaveAll? may clear generation counter!
		Out.Write(EntityID.Raw);
		DEM::BinaryFormat::Serialize(Out, Entity);
	}

	Out.Write(static_cast<uint32_t>(_Storages.size()));
	for (const auto& [ComponentID, Storage] : _StorageMap)
	{
		Out.Write(ComponentID);
		Storage->SaveAll(Out);
	}

	return true;
}
//---------------------------------------------------------------------

bool CGameWorld::SaveDiff(Data::CParams& Out)
{
	if (_State == EState::BaseLoaded) return false;

	// Save entities deleted from the level as explicit nulls
	for (const auto& BaseEntity : _EntitiesBase)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _EntitiesBase.GetHandle(&BaseEntity);
		if (!GetEntity(EntityID))
			Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()), Data::CData());
	}

	// Save modified / added entities and their components
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);
		SaveEntityToParams(Out, EntityID, Entity, _EntitiesBase.GetValue(EntityID));
	}

	return true;
}
//---------------------------------------------------------------------

bool CGameWorld::SaveDiff(IO::CBinaryWriter& Out)
{
	if (_State == EState::BaseLoaded) return false;

	// Save a list of entities deleted from the level
	for (const auto& BaseEntity : _EntitiesBase)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _EntitiesBase.GetHandle(&BaseEntity);
		if (!GetEntity(EntityID))
			Out << EntityID.Raw;
	}

	// End the list of deleted entities with an invalid handle (much like a trailing \0)
	Out << CEntityStorage::INVALID_HANDLE_VALUE;

	// Save diffs for all changed and added entities in actual list
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get EntityID for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		const auto CurrPos = Out.GetStream().Tell();
		Out << EntityID.Raw;

		bool HasDiff;
		if (auto pBaseEntity = _EntitiesBase.GetValue(EntityID))
		{
			// Existing entity, save modified part
			n_assert2(Entity.TemplateID == pBaseEntity->TemplateID, "Entity template must never change in runtime!");
			HasDiff = DEM::BinaryFormat::SerializeDiff(Out, Entity, *pBaseEntity);
		}
		else
		{
			// New entity, save diff from default-created entity (better than full data!)
			HasDiff = DEM::BinaryFormat::SerializeDiff(Out, Entity, CEntity{});
		}

		if (!HasDiff)
		{
			// "Unwrite" entity ID and any diff data written ahead
			Out.GetStream().Seek(CurrPos, IO::Seek_Begin);
			Out.GetStream().Truncate();
		}
	}

	// End the list of new and modified entities
	Out << CEntityStorage::INVALID_HANDLE_VALUE;

	// Save component diffs per storage
	Out.Write(static_cast<uint32_t>(_Storages.size()));
	for (const auto& [ComponentID, Storage] : _StorageMap)
	{
		Out.Write(ComponentID);
		Storage->SaveDiff(Out);
	}

	return true;
}
//---------------------------------------------------------------------

IO::IStream* CGameWorld::GetBaseStream(U64 Offset) const
{
	if (!_BaseStream || Offset >= _BaseStream->GetSize()) return nullptr;
	if (!_BaseStream->Seek(static_cast<I64>(Offset), IO::Seek_Begin)) return nullptr;
	return _BaseStream.Get();
}
//---------------------------------------------------------------------

// FIXME: make difference between 'non-interactive' and 'interactive same as whole'. AABB::Empty + AABB::Invalid?
CGameLevel* CGameWorld::CreateLevel(CStrID ID, const CAABB& Bounds, const CAABB& InteractiveBounds, UPTR SubdivisionDepth)
{
	// Ensure there is no level with same ID

	auto Level = n_new(DEM::Game::CGameLevel(ID, Bounds, InteractiveBounds, SubdivisionDepth));

	// Add level to the list

	// Notify level activated?

	// Return level ptr

	return nullptr;
}
//---------------------------------------------------------------------

CGameLevel* CGameWorld::LoadLevel(CStrID ID, const Data::CParams& In)
{
	if (FindLevel(ID))
	{
		::Sys::Error("CGameWorld::LoadLevel() > level already exists");
		return nullptr;
	}

	vector3 Center(vector3::Zero);
	vector3 Size(512.f, 128.f, 512.f);
	int SubdivisionDepth = 0;

	In.TryGet(Center, CStrID("Center"));
	In.TryGet(Size, CStrID("Size"));
	In.TryGet(SubdivisionDepth, CStrID("SubdivisionDepth"));
	vector3 InteractiveCenter = In.Get(CStrID("InteractiveCenter"), Center);
	vector3 InteractiveSize = In.Get(CStrID("InteractiveSize"), Size);

	CGameLevel* pLevel;
	{
		PGameLevel Level = n_new(DEM::Game::CGameLevel(ID, CAABB(Center, Size * 0.5f), CAABB(InteractiveCenter, InteractiveSize * 0.5f), SubdivisionDepth));
		pLevel = _Levels.emplace(ID, std::move(Level)).first->second.Get();
		if (!pLevel) return nullptr;
	}

	// Load optional scene with static graphics, collision and other attributes. No entity is associated with it.
	auto StaticSceneID = In.Get(CStrID("StaticScene"), CString::Empty);
	if (!StaticSceneID.IsEmpty())
	{
		// This resource can be unloaded by the client code when reloading it in the near future is not expected.
		// The most practical way is to check resources with refcount = 1, they are held by a resource manager only.
		// Use StaticSceneIsUnique = false if you expect to use the scene in multuple level instances and you
		// plan to modify it in the runtime (which is not recommended nor typical for _static_ scenes).
		auto Rsrc = _ResMgr.RegisterResource<Scene::CSceneNode>(CStrID(StaticSceneID.CStr()));
		if (auto StaticSceneNode = Rsrc->ValidateObject<Scene::CSceneNode>())
		{
			if (In.Get(CStrID("StaticSceneIsUnique"), true))
			{
				// Unregister unique scene from resources to prevent unintended reuse which can cause huge problems
				pLevel->GetSceneRoot().AddChild(CStrID("StaticScene"), *StaticSceneNode);
				_ResMgr.UnregisterResource(Rsrc->GetUID());
			}
			else
			{
				pLevel->GetSceneRoot().AddChild(CStrID("StaticScene"), *StaticSceneNode->Clone());
			}
		}
	}

	// Load navigation map, if present
	auto NavigationMapID = In.Get(CStrID("NavigationMap"), CString::Empty);
	if (!NavigationMapID.IsEmpty() && pLevel->GetAI())
	{
		n_verify(pLevel->GetAI()->LoadNavMesh(NavigationMapID.CStr()));
	}

	Data::PParams EntitiesDesc;
	if (In.TryGet(EntitiesDesc, CStrID("Entities")))
		LoadBase(*EntitiesDesc);

	// Notify level loaded / activated, validate if automatic

	return pLevel;
}
//---------------------------------------------------------------------

CGameLevel* CGameWorld::FindLevel(CStrID ID) const
{
	auto It = _Levels.find(ID);
	return (It == _Levels.cend()) ? nullptr : It->second.Get();
}
//---------------------------------------------------------------------

void CGameWorld::ValidateLevel(CStrID LevelID)
{
	for (const auto& Storage : _Storages)
		Storage->ValidateComponents(LevelID);
}
//---------------------------------------------------------------------

void CGameWorld::InvalidateLevel(CStrID LevelID)
{
	for (const auto& Storage : _Storages)
		Storage->InvalidateComponents(LevelID);
}
//---------------------------------------------------------------------

bool CGameWorld::InstantiateTemplate(HEntity EntityID, CStrID TemplateID, bool BaseState, bool Validate)
{
	if (!EntityID || !TemplateID) return false;

	auto Rsrc = _ResMgr.RegisterResource<CEntityTemplate>(TemplateID);
	auto pTpl = Rsrc ? Rsrc->ValidateObject<CEntityTemplate>() : nullptr;
	n_assert2(pTpl, ("CGameWorld::InstantiateTemplate() > can't load requested template " + TemplateID.ToString()).c_str());
	if (!pTpl) return false;

	// Existing components override the template
	for (const auto& Param : pTpl->GetDesc())
	{
		// Template can't request removal of a component
		n_assert_dbg(!Param.GetRawValue().IsVoid());

		if (auto pStorage = FindComponentStorage(Param.GetName()))
			pStorage->InstantiateTemplate(EntityID, BaseState, Validate);
	}

	return true;
}
//---------------------------------------------------------------------

HEntity CGameWorld::CreateEntity(CStrID LevelID, CStrID TemplateID)
{
	auto EntityID = _Entities.Allocate();
	auto pEntity = _Entities.GetValueUnsafe(EntityID);
	if (!pEntity) return CEntityStorage::INVALID_HANDLE;

	pEntity->LevelID = LevelID;
	pEntity->Level = FindLevel(LevelID);
	pEntity->TemplateID = TemplateID; // Used inside InstantiateTemplate // FIXME: pass template data into pStorage->InstantiateTemplate instead of Validate flag?

	if (!InstantiateTemplate(EntityID, TemplateID, false, true))
		pEntity->TemplateID = CStrID::Empty;

	return EntityID;
}
//---------------------------------------------------------------------

void CGameWorld::DeleteEntity(HEntity EntityID)
{
	if (!_Entities.Free(EntityID)) return;
	for (auto& Storage : _Storages)
		Storage->RemoveComponent(EntityID);
}
//---------------------------------------------------------------------

const IComponentStorage* CGameWorld::FindComponentStorage(CStrID ComponentName) const
{
	auto It = _StorageMap.find(ComponentName);
	return (It == _StorageMap.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

IComponentStorage* CGameWorld::FindComponentStorage(CStrID ComponentName)
{
	auto It = _StorageMap.find(ComponentName);
	return (It == _StorageMap.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

}