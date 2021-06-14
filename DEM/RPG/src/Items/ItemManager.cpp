#include "ItemManager.h"
#include <Items/ItemComponent.h>
#include <Items/ItemStackComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Game/ECS/Components/SceneComponent.h>

namespace DEM::RPG
{

CItemManager::CItemManager(Game::CGameSession& Owner) : _Session(Owner)
{
}
//---------------------------------------------------------------------

void CItemManager::GatherExistingTemplates()
{
	_Templates.clear();

	auto pWorld = _Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return;

	// All item templates live in no level (CStrID::Empty).
	// For iteration to work, no-level components must be validated before calling GatherExistingTemplates().
	pWorld->ForEachEntityInLevelWith<const CItemComponent>(CStrID::Empty,
		[this](auto EntityID, auto& Entity, const CItemComponent& Item)
	{
		if (Entity.TemplateID) _Templates.emplace(Entity.TemplateID, EntityID);

		//!!!if the same template is already prototyped, should delete duplicates and patch proto IDs in stacks!
	});

	//???really need? allows to process non-validated prototypes, but doesn't process ones without alive instances
	pWorld->ForEachComponent<const CItemStackComponent>([this, pWorld](auto EntityID, const CItemStackComponent& Stack)
	{
		if (!Stack.Prototype || Stack.Prototype == EntityID) return;

		const Game::CEntity* pProtoEntity = pWorld->GetEntity(Stack.Prototype);
		if (!pProtoEntity) return;

		if (pProtoEntity->TemplateID) _Templates.emplace(pProtoEntity->TemplateID, Stack.Prototype);
	});
}
//---------------------------------------------------------------------

Game::HEntity CItemManager::InternalCreateStack(Game::CGameWorld& World, CStrID LevelID, CStrID ItemID, U32 Count, Game::HEntity Container)
{
	Game::HEntity ProtoEntity;
	auto It = _Templates.find(ItemID);
	if (It == _Templates.cend())
	{
		ProtoEntity = World.CreateEntity(CStrID::Empty, ItemID);
		//World.ValidateComponents() !!!only for the new entity! it is needed to create template components!
		_Templates.emplace(ItemID, ProtoEntity);
	}
	else ProtoEntity = It->second;

	Game::HEntity StackEntity = World.CreateEntity(LevelID);
	auto pStack = World.AddComponent<CItemStackComponent>(StackEntity);
	if (!pStack)
	{
		World.DeleteEntity(StackEntity);
		return {};
	}

	pStack->Prototype = ProtoEntity;
	pStack->Count = Count;
	pStack->Container = Container;

	return StackEntity;
}
//---------------------------------------------------------------------

Game::HEntity CItemManager::CreateStack(CStrID ItemID, U32 Count, Game::HEntity Container)
{
	auto pWorld = _Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return {};

	const Game::CEntity* pContainerEntity = pWorld->GetEntity(Container);
	if (!pContainerEntity) return {};

	return InternalCreateStack(*pWorld, pContainerEntity->LevelID, ItemID, Count, Container);
}
//---------------------------------------------------------------------

Game::HEntity CItemManager::CreateStack(CStrID ItemID, U32 Count, CStrID LevelID, const Math::CTransformSRT& WorldTfm)
{
	auto pWorld = _Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return {};

	auto StackEntity = InternalCreateStack(*pWorld, LevelID, ItemID, Count, {});
	if (!StackEntity) return {};

	auto pSceneComponent = pWorld->AddComponent<Game::CSceneComponent>(StackEntity);
	if (!pSceneComponent)
	{
		pWorld->DeleteEntity(StackEntity);
		return {};
	}

	//!!!add in-location item scene object!

	pSceneComponent->RootNode->SetLocalTransform(WorldTfm);

	return StackEntity;
}
//---------------------------------------------------------------------

}
