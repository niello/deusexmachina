#include "GameWorld.h"
#include <Game/GameLevel.h>
#include <AI/AILevel.h>
#include <Scene/SceneNode.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>

namespace DEM::Game
{

CGameWorld::CGameWorld(Resources::CResourceManager& ResMgr)
	: _ResMgr(ResMgr)
{
	//???register default loader for entity template?
}
//---------------------------------------------------------------------

void CGameWorld::SaveEntities(CStrID LevelID, Data::CParams& Out) const
{
	auto pLevel = FindLevel(LevelID);
	if (!pLevel) return;

	for (const auto& Entity : _Entities)
	{
		if (Entity.Level != pLevel) continue;

		// FIXME: must get for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		Data::PParams SEntity = n_new(Data::CParams());
		SEntity->Set(CStrID("ID"), static_cast<int>(EntityID.Raw));
		if (Entity.TemplateID) SEntity->Set(CStrID("Tpl"), Entity.TemplateID);
		if (!Entity.IsActive) SEntity->Set(CStrID("Active"), false);

		// TODO: if tpl present, save diff?
		for (const auto& [ComponentID, Storage] : _StorageMap)
		{
			Data::CData SComponent;
			Storage->SaveComponentToParams(EntityID, SComponent);
			SEntity->Set(ComponentID, std::move(SComponent));
		}

		if (Entity.Name)
			Out.Set(Entity.Name, std::move(SEntity));
		else
			Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()), std::move(SEntity));
	}
}
//---------------------------------------------------------------------

void CGameWorld::LoadEntities(CStrID LevelID, const Data::CParams& In)
{
	auto pLevel = FindLevel(LevelID);
	if (!pLevel)
	{
		::Sys::Error("CGameWorld::LoadEntities() > level doesn't exist");
		return;
	}

	const CStrID sidID("ID");
	const CStrID sidTpl("Tpl");
	const CStrID sidActive("Active");

	for (const auto& Param : In)
	{
		const auto& SEntity = *Param.GetValue<Data::PParams>();
		auto RawHandle = SEntity.Get<int>(sidID, CEntityStorage::INVALID_HANDLE_VALUE);

		CEntity NewEntity;
		NewEntity.Level = pLevel;
		NewEntity.Name = Param.GetName();
		NewEntity.TemplateID = SEntity.Get<CStrID>(sidTpl, CStrID::Empty);
		NewEntity.IsActive = SEntity.Get(sidActive, true);

		auto EntityID = _Entities.AllocateWithHandle(static_cast<CEntityStorage::THandleValue>(RawHandle), std::move(NewEntity));
		if (!EntityID) continue;

		// TODO: instatiate template, then merge state from saved components

		for (const auto& Param : SEntity)
			if (auto pStorage = FindComponentStorage(Param.GetName()))
				pStorage->LoadComponentFromParams(EntityID, Param.GetRawValue());
	}
}
//---------------------------------------------------------------------

void CGameWorld::SaveEntitiesDiff(CStrID LevelID, Data::CParams& Out, const CGameWorld& Base) const
{
	auto pBaseLevel = Base.FindLevel(LevelID);
	if (!pBaseLevel)
	{
		// If base world has no such level, the diff will be a whole level
		SaveEntities(LevelID, Out);
		return;
	}

	auto pLevel = FindLevel(LevelID);
	if (!pLevel) return;

	// Save entities deleted from the level as explicit nulls
	for (const auto& BaseEntity : Base.GetEntities())
	{
		if (BaseEntity.Level != pBaseLevel) continue;

		// FIXME: must get for free when iterating an array
		auto EntityID = Base.GetEntities().GetHandle(&BaseEntity);

		if (auto pEntity = GetEntity(EntityID))
			if (pEntity->Level == pLevel) continue;

		Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()), Data::CData());
	}

	for (const auto& Entity : _Entities)
	{
		if (Entity.Level != pLevel) continue;

		// FIXME: must get for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		Data::PParams SEntity;

		auto pBaseEntity = Base.GetEntity(EntityID);
		if (pBaseEntity && pBaseEntity->Level == pBaseLevel)
		{
			// Existing entity, save modified part
			n_assert2(Entity.TemplateID == pBaseEntity->TemplateID, "Entity template must never change in runtime!");
			if (Entity.IsActive != pBaseEntity->IsActive) SEntity->Set(CStrID("Active"), Entity.IsActive);
			for (const auto& [ComponentID, Storage] : _StorageMap)
			{
				Data::CData SComponent;
				if (Storage->SaveComponentDiffToParams(EntityID, SComponent, Base.FindComponentStorage(ComponentID)))
				{
					if (!SEntity) SEntity = n_new(Data::CParams());
					SEntity->Set(ComponentID, std::move(SComponent));
				}
			}
		}
		else
		{
			// New entity, save full data
			// TODO: if tpl present, save diff?
			SEntity = n_new(Data::CParams());
			if (Entity.TemplateID) SEntity->Set(CStrID("Tpl"), Entity.TemplateID);
			if (!Entity.IsActive) SEntity->Set(CStrID("Active"), false);
			for (const auto& [ComponentID, Storage] : _StorageMap)
			{
				Data::CData SComponent;
				Storage->SaveComponentToParams(EntityID, SComponent);
				SEntity->Set(ComponentID, std::move(SComponent));
			}
		}

		// Save entity only if something changed
		if (SEntity)
		{
			// NB: TemplateID can't change, so it is saved only with full data
			SEntity->Set(CStrID("ID"), static_cast<int>(EntityID.Raw));

			if (Entity.Name)
				Out.Set(Entity.Name, std::move(SEntity));
			else
				Out.Set(CStrID(("__" + std::to_string(EntityID.Raw)).c_str()), std::move(SEntity));
		}
	}
}
//---------------------------------------------------------------------

void CGameWorld::LoadEntitiesDiff(CStrID LevelID, const Data::CParams& In)
{
	auto pLevel = FindLevel(LevelID);
	if (!pLevel)
	{
		::Sys::Error("CGameWorld::LoadEntitiesDiff() > level doesn't exist");
		return;
	}

	const CStrID sidID("ID");
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
			auto pEntity = _Entities.GetValue(EntityID);
			if (pEntity)
			{
				pEntity->Name = EntityParam.GetName();
				SEntity.Get<bool>(pEntity->IsActive, sidActive);
			}
			else
			{
				CEntity NewEntity;
				NewEntity.Level = pLevel;
				NewEntity.Name = EntityParam.GetName();
				NewEntity.TemplateID = SEntity.Get<CStrID>(sidTpl, CStrID::Empty);
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

void CGameWorld::SaveEntities(CStrID LevelID, IO::CBinaryWriter& Out) const
{
	auto pLevel = FindLevel(LevelID);
	if (!pLevel) return;

	Out.Write(static_cast<uint32_t>(_Entities.size()));
	for (const auto& Entity : _Entities)
	{
		if (Entity.Level != pLevel) continue;

		// FIXME: must get for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		Out.Write(EntityID.Raw);
		Out.Write(Entity.Name);
		Out.Write(Entity.TemplateID);
		Out.Write(Entity.IsActive);
	}

	// TODO: save components only for entities without TemplateID
	//???unordered set of IDs with tpls? or map to Tpl data? To check the component presence.

	Out.Write(static_cast<uint32_t>(_Storages.size()));
	for (const auto& [ComponentID, Storage] : _StorageMap)
	{
		// FIXME: LevelID!!!
		// if (Entity.Level != pLevel) continue;

		Out.Write(ComponentID);
		Storage->SaveAllComponentsToBinary(Out);
	}

	// TODO: for modified templated entities save diff between tpl components and actual components (optional)
	// Don't process added components that aren't present in the template.
}
//---------------------------------------------------------------------

void CGameWorld::LoadEntities(CStrID LevelID, IO::CBinaryReader& In)
{
	auto pLevel = FindLevel(LevelID);
	if (!pLevel)
	{
		::Sys::Error("CGameWorld::LoadEntities() > level doesn't exist");
		return;
	}

	const auto EntityCount = In.Read<uint32_t>();
	for (uint32_t i = 0; i < EntityCount; ++i)
	{
		const auto EntityIDRaw = In.Read<CEntityStorage::THandleValue>();
		CEntity NewEntity;
		NewEntity.Level = pLevel;
		In.Read(NewEntity.Name);
		In.Read(NewEntity.TemplateID);
		In.Read(NewEntity.IsActive);
		const auto EntityID = _Entities.AllocateWithHandle(EntityIDRaw, std::move(NewEntity));
		n_assert_dbg(EntityID);
	}

	// Components that come from templates aren't saved and therefore won't be loaded here
	const auto ComponentTypeCount = In.Read<uint32_t>();
	for (uint32_t i = 0; i < ComponentTypeCount; ++i)
	{
		const auto TypeID = In.Read<CStrID>();
		if (auto pStorage = FindComponentStorage(TypeID))
			pStorage->LoadAllComponentsFromBinary(In);
	}

	for (auto& Entity : _Entities)
	{
		if (!Entity.TemplateID || Entity.Level != pLevel) continue;

		// TODO: create components from template for this entity, don't erase existing entity components
	}

	// TODO: load diff part to modify templated components (optional, for per-entity tpl modification)
}
//---------------------------------------------------------------------

void CGameWorld::SaveEntitiesDiff(CStrID LevelID, IO::CBinaryWriter& Out, const CGameWorld& Base) const
{
	auto pBaseLevel = Base.FindLevel(LevelID);
	if (!pBaseLevel)
	{
		// If base world has no such level, the diff will be a whole level
		SaveEntities(LevelID, Out);
		return;
	}

	auto pLevel = FindLevel(LevelID);
	if (!pLevel) return;

	// Save a list of entities deleted from the level
	for (const auto& BaseEntity : Base.GetEntities())
	{
		if (BaseEntity.Level != pBaseLevel) continue;

		// FIXME: must get for free when iterating an array
		auto EntityID = Base.GetEntities().GetHandle(&BaseEntity);

		if (auto pEntity = GetEntity(EntityID))
			if (pEntity->Level == pLevel) continue;

		Out << EntityID.Raw;
	}

	// End the list of deleted entities with an invalid handle (much like a trailing \0)
	Out << CEntityStorage::INVALID_HANDLE_VALUE;

	for (const auto& Entity : _Entities)
	{
		if (Entity.Level != pLevel) continue;

		// FIXME: must get for free when iterating an array
		auto EntityID = _Entities.GetHandle(&Entity);

		Out << EntityID.Raw;

		auto pBaseEntity = Base.GetEntity(EntityID);
		if (pBaseEntity && pBaseEntity->Level == pBaseLevel)
		{
			// Existing entity, save modified part
			n_assert2(Entity.TemplateID == pBaseEntity->TemplateID, "Entity template must never change in runtime!");
			DEM::BinaryFormat::SerializeDiff(Out, Entity, *pBaseEntity);
		}
		else
		{
			// New entity, save diff from default-created entity (better than full data!)
			DEM::BinaryFormat::SerializeDiff(Out, Entity, CEntity{});
		}
	}

	// End the list of new and modified entities
	Out << CEntityStorage::INVALID_HANDLE_VALUE;

	// Save component diffs per storage
	Out.Write(static_cast<uint32_t>(_Storages.size()));
	for (const auto& [ComponentID, Storage] : _StorageMap)
	{
		Out.Write(ComponentID);
		Storage->SaveAllComponentsDiffToBinary(Out, Base.FindComponentStorage(ComponentID));
	}
}
//---------------------------------------------------------------------

void CGameWorld::LoadEntitiesDiff(CStrID LevelID, IO::CBinaryReader& In)
{
/*
	auto pLevel = FindLevel(LevelID);
	if (!pLevel)
	{
		::Sys::Error("CGameWorld::LoadEntitiesDiff() > level doesn't exist");
		return;
	}

	const CStrID sidID("ID");
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
			auto pEntity = _Entities.GetValue(EntityID);
			if (!pEntity)
			{
				CEntity NewEntity;
				NewEntity.Level = pLevel;
				NewEntity.Name = EntityParam.GetName();
				NewEntity.TemplateID = SEntity.Get<CStrID>(sidTpl, CStrID::Empty);
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
*/
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

	In.Get(Center, CStrID("Center"));
	In.Get(Size, CStrID("Size"));
	In.Get(SubdivisionDepth, CStrID("SubdivisionDepth"));
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
	if (In.Get(EntitiesDesc, CStrID("Entities")))
		LoadEntities(ID, *EntitiesDesc);

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
	if (auto pLevel = FindLevel(LevelID))
	{
		auto EntityID = _Entities.Allocate();
		if (auto pEntity = _Entities.GetValueUnsafe(EntityID))
		{
			pEntity->Level = pLevel;
			return EntityID;
		}
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