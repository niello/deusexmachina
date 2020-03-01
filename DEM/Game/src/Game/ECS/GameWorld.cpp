#include "GameWorld.h"
#include <Game/GameLevel.h>
#include <AI/AILevel.h>
#include <Scene/SceneNode.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <unordered_set>

// TODO: entity templates!

namespace DEM::Game
{

CGameWorld::CGameWorld(Resources::CResourceManager& ResMgr)
	: _ResMgr(ResMgr)
{
}
//---------------------------------------------------------------------

void CGameWorld::LoadBase(const Data::CParams& In)
{
	// CParams source doesn't support on-demand base access, diff will be populated
	// with the whole world data. It is acceptable for debug-only functionality.
	_BaseStream.Reset();

	// Erase all previous data in the world
	_EntitiesBase.Clear(In.GetCount());
	_Entities.Clear(In.GetCount());
	// FIXME: erase components

	const CStrID sidID("ID");
	const CStrID sidLevel("Level");
	const CStrID sidTpl("Tpl");
	const CStrID sidActive("Active");

	for (const auto& Param : In)
	{
		const auto& SEntity = *Param.GetValue<Data::PParams>();
		auto RawHandle = SEntity.Get<int>(sidID, CEntityStorage::INVALID_HANDLE_VALUE);

		CEntity NewEntity;
		NewEntity.LevelID = SEntity.Get<CStrID>(sidLevel, CStrID::Empty);
		NewEntity.TemplateID = SEntity.Get<CStrID>(sidTpl, CStrID::Empty);
		NewEntity.Name = Param.GetName();
		NewEntity.IsActive = SEntity.Get(sidActive, true);

		auto EntityID = _Entities.AllocateWithHandle(static_cast<CEntityStorage::THandleValue>(RawHandle), std::move(NewEntity));
		if (!EntityID) continue;

		for (const auto& Param : SEntity)
			if (auto pStorage = FindComponentStorage(Param.GetName()))
				pStorage->LoadComponentFromParams(EntityID, Param.GetRawValue());
	}
}
//---------------------------------------------------------------------

void CGameWorld::LoadBase(IO::PStream InStream)
{
	if (!InStream) return;

	IO::CBinaryReader In(*InStream);

	const auto EntityCount = In.Read<uint32_t>();

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

		//!!!DBG TMP!
		const auto EntityID2 = _Entities.AllocateWithHandle(EntityIDRaw, NewEntity);
		n_assert_dbg(EntityID2 && EntityID2 == HEntity{ EntityIDRaw });

		const auto EntityID = _EntitiesBase.AllocateWithHandle(EntityIDRaw, std::move(NewEntity));
		n_assert_dbg(EntityID && EntityID == HEntity{ EntityIDRaw });
	}

	// FIXME: decide!
	// leave actual list empty for now (or just copy? don't want to copy because diff may be loaded next)
	//???store loading state (none, base, diff) to avoid duplicate work and partially uninitialized state?

	const auto ComponentTypeCount = In.Read<uint32_t>();
	for (uint32_t i = 0; i < ComponentTypeCount; ++i)
	{
		const auto TypeID = In.Read<CStrID>();
		if (auto pStorage = FindComponentStorage(TypeID))
			pStorage->LoadBase(In);
	}
}
//---------------------------------------------------------------------

void CGameWorld::LoadDiff(const Data::CParams& In)
{
	const CStrID sidID("ID");
	const CStrID sidLevel("Level");
	const CStrID sidTpl("Tpl");
	const CStrID sidActive("Active");

	for (const auto& EntityParam : In)
	{
		if (EntityParam.GetRawValue().IsVoid())
		{
			// Deleted entity is always named as __UID. Skip leading "__".
			auto RawHandle = static_cast<CEntityStorage::THandleValue>(std::atoi(EntityParam.GetName().CStr() + 2));
			DeleteEntity({ RawHandle });
		}
		else
		{
			const auto& SEntity = *EntityParam.GetValue<Data::PParams>();
			auto RawHandle = static_cast<CEntityStorage::THandleValue>(SEntity.Get<int>(sidID, CEntityStorage::INVALID_HANDLE_VALUE));

			HEntity EntityID{ RawHandle };
			if (auto pEntity = _Entities.GetValue(EntityID))
			{
				pEntity->Name = EntityParam.GetName();
				SEntity.TryGet<CStrID>(pEntity->LevelID, sidLevel);
				SEntity.TryGet<bool>(pEntity->IsActive, sidActive);
			}
			else
			{
				CEntity NewEntity;
				NewEntity.LevelID = SEntity.Get<CStrID>(sidLevel, CStrID::Empty);
				NewEntity.TemplateID = SEntity.Get<CStrID>(sidTpl, CStrID::Empty);
				NewEntity.Name = EntityParam.GetName();
				NewEntity.IsActive = SEntity.Get(sidActive, true);

				EntityID = _Entities.AllocateWithHandle(RawHandle, std::move(NewEntity));
				pEntity = _Entities.GetValue(EntityID);
				if (!pEntity) continue;

				// TODO: instatiate template, then merge state from saved components
			}

			for (const auto& ComponentParam : SEntity)
			{
				auto pStorage = FindComponentStorage(ComponentParam.GetName());
				if (!pStorage) continue;

				if (ComponentParam.GetRawValue().IsVoid())
					pStorage->RemoveComponent(EntityID);
				else
					pStorage->LoadComponentFromParams(EntityID, ComponentParam.GetRawValue());
			}
		}
	}
}
//---------------------------------------------------------------------

void CGameWorld::LoadDiff(IO::PStream InStream)
{
	if (!InStream) return;

	IO::CBinaryReader In(*InStream);

	// Clear previous diff info, keep base intact
	if (_Entities.size()) _Entities.Clear(_EntitiesBase.size());

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
		// FIXME: must get for free when iterating an array
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
}
//---------------------------------------------------------------------

void CGameWorld::SaveAll(Data::CParams& Out)
{
	const CStrID sidID("ID");
	const CStrID sidLevel("Level");
	const CStrID sidTpl("Tpl");
	const CStrID sidActive("Active");

	for (const auto& Entity : _Entities)
	{
		// FIXME: must get for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		Data::PParams SEntity = n_new(Data::CParams(4));
		SEntity->Set(sidID, static_cast<int>(EntityID.Raw));
		SEntity->Set(sidLevel, Entity.LevelID);
		if (Entity.TemplateID) SEntity->Set(sidTpl, Entity.TemplateID);
		if (!Entity.IsActive) SEntity->Set(sidActive, false);

		for (const auto& [ComponentID, Storage] : _StorageMap)
		{
			Data::CData SComponent;
			if (Storage->SaveComponentToParams(EntityID, SComponent))
				SEntity->Set(ComponentID, std::move(SComponent));
		}

		if (Entity.Name)
			Out.Set(Entity.Name, std::move(SEntity));
		else
			Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()), std::move(SEntity));
	}
}
//---------------------------------------------------------------------

void CGameWorld::SaveAll(IO::CBinaryWriter& Out)
{
	Out.Write(static_cast<uint32_t>(_Entities.size()));
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get for free when iterating an array
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
}
//---------------------------------------------------------------------

void CGameWorld::SaveDiff(Data::CParams& Out)
{
	const CStrID sidID("ID");
	const CStrID sidLevel("Level");
	const CStrID sidTpl("Tpl");
	const CStrID sidActive("Active");

	// Save entities deleted from the level as explicit nulls
	for (const auto& BaseEntity : _EntitiesBase)
	{
		// FIXME: must get for free when iterating an array
		auto EntityID = _EntitiesBase.GetHandle(&BaseEntity);
		if (!GetEntity(EntityID))
			Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()), Data::CData());
	}

	// Save modified / added entities and their components
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		Data::PParams SEntity;

		if (auto pBaseEntity = _EntitiesBase.GetValue(EntityID))
		{
			// Existing entity, save modified part
			n_assert2(Entity.TemplateID == pBaseEntity->TemplateID, "Entity template must never change in runtime!");
			if (Entity.LevelID != pBaseEntity->LevelID) SEntity->Set(sidLevel, Entity.LevelID);
			if (Entity.IsActive != pBaseEntity->IsActive) SEntity->Set(sidActive, Entity.IsActive);
			for (const auto& [ComponentID, Storage] : _StorageMap)
			{
				Data::CData SComponent;
				if (Storage->SaveComponentDiffToParams(EntityID, SComponent))
				{
					if (!SEntity) SEntity = n_new(Data::CParams());
					SEntity->Set(ComponentID, std::move(SComponent));
				}
			}
		}
		else
		{
			// New entity, save full data
			SEntity = n_new(Data::CParams());
			if (Entity.LevelID) SEntity->Set(sidLevel, Entity.LevelID);
			if (Entity.TemplateID) SEntity->Set(sidTpl, Entity.TemplateID);
			if (!Entity.IsActive) SEntity->Set(sidActive, false);
			for (const auto& [ComponentID, Storage] : _StorageMap)
			{
				Data::CData SComponent;
				if (Storage->SaveComponentToParams(EntityID, SComponent))
					SEntity->Set(ComponentID, std::move(SComponent));
			}
		}

		// Save entity only if something changed
		if (SEntity)
		{
			SEntity->Set(sidID, static_cast<int>(EntityID.Raw));

			if (Entity.Name)
				Out.Set(Entity.Name, std::move(SEntity));
			else
				Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()), std::move(SEntity));
		}
	}
}
//---------------------------------------------------------------------

void CGameWorld::SaveDiff(IO::CBinaryWriter& Out)
{
	// Save a list of entities deleted from the level
	for (const auto& BaseEntity : _EntitiesBase)
	{
		// FIXME: must get for free when iterating an array
		auto EntityID = _EntitiesBase.GetHandle(&BaseEntity);
		if (!GetEntity(EntityID))
			Out << EntityID.Raw;
	}

	// End the list of deleted entities with an invalid handle (much like a trailing \0)
	Out << CEntityStorage::INVALID_HANDLE_VALUE;

	// Save diffs for all changed and added entities in actual list
	for (const auto& Entity : _Entities)
	{
		// FIXME: must get for free when iterating an array
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
			// "Unwrite" entity ID
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
}
//---------------------------------------------------------------------

IO::IStream* CGameWorld::GetBaseStream(U64 Offset) const
{
	if (!_BaseStream || Offset >= _BaseStream->GetSize()) return nullptr;
	if (!_BaseStream->Seek(static_cast<I64>(Offset), IO::Seek_Begin)) return nullptr;
	return _BaseStream.Get();
}
//---------------------------------------------------------------------

//!!!DBG TMP! Probably not a part of the final API
void CGameWorld::PrepareEntities(CStrID LevelID)
{
	for (const auto& Storage : _Storages)
		Storage->PrepareComponents(LevelID);
}
void CGameWorld::UnloadEntities(CStrID LevelID)
{
	for (const auto& Storage : _Storages)
		Storage->UnloadComponents(LevelID);
}

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

HEntity CGameWorld::CreateEntity(CStrID LevelID)
{
	auto EntityID = _Entities.Allocate();
	if (auto pEntity = _Entities.GetValueUnsafe(EntityID))
	{
		pEntity->LevelID = LevelID;
		pEntity->Level = FindLevel(LevelID);
		return EntityID;
	}

	return CEntityStorage::INVALID_HANDLE;
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