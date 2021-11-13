#pragma once
#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <Game/Interaction/InteractionContext.h>
#include <Scene/SceneComponent.h>
#include <Physics/CollisionAttribute.h>
#include <Physics/CollisionShape.h>
#include <Physics/RigidBodyComponent.h>
#include <Physics/CharacterControllerComponent.h>
#include <Resources/ResourceManager.h>

namespace DEM::Game
{
void UpdateCharacterControllerShape(CCharacterControllerComponent& Character);

void SetupPassiveColliders(HEntity EntityID, CSceneComponent& SceneComponent, Physics::CPhysicsLevel* pPhysicsLevel)
{
	if (!SceneComponent.RootNode) return;

	SceneComponent.RootNode->Visit([EntityID, pPhysicsLevel](Scene::CSceneNode& Node)
	{
		for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
		{
			if (auto pCollAttr = Node.GetAttribute(i)->As<Physics::CCollisionAttribute>())
			{
				pCollAttr->SetPhysicsLevel(pPhysicsLevel);
				pCollAttr->GetCollider()->UserData() = std::pair(EntityID, pCollAttr);
			}
		}
		return true;
	});
}
//---------------------------------------------------------------------

bool GetTargetFromPhysicsObject(const Physics::CPhysicsObject& Object, CTargetInfo& OutTarget)
{
	OutTarget.Valid = false;

	if (Object.UserData().has_value())
	{
		if (auto pRB = Object.As<const Physics::CRigidBody>())
		{
			if (auto pHEntity = std::any_cast<DEM::Game::HEntity>(&Object.UserData()))
				OutTarget.Entity = *pHEntity;
			OutTarget.pNode = pRB->GetControlledNode();
			OutTarget.Valid = true;
		}
		else
		{
			if (auto pPair = std::any_cast<std::pair<DEM::Game::HEntity, Physics::CCollisionAttribute*>>(&Object.UserData()))
			{
				OutTarget.Entity = pPair->first;
				OutTarget.pNode = pPair->second->GetNode();
				OutTarget.Valid = true;
			}
		}
	}

	return OutTarget.Valid;
}
//---------------------------------------------------------------------

void InitPhysicsObjects(CGameWorld& World, CStrID LevelID, Resources::CResourceManager& RsrcMgr)
{
	auto pLevel = World.FindLevel(LevelID);
	if (!pLevel) return;

	auto pPhysicsLevel = pLevel->GetPhysics();
	if (!pPhysicsLevel) return;

	// Setup rigid bodies
	World.ForEachEntityInLevelWith<CRigidBodyComponent, CSceneComponent>(
		LevelID,
		[pPhysicsLevel, &RsrcMgr](auto EntityID, auto& Entity,
			CRigidBodyComponent& RigidBodyComponent,
			CSceneComponent& SceneComponent)
	{
		if (!RigidBodyComponent.ShapeAssetID) return;

		auto Rsrc = RsrcMgr.RegisterResource<Physics::CCollisionShape>(RigidBodyComponent.ShapeAssetID.CStr());
		if (auto Shape = Rsrc->ValidateObject<Physics::CCollisionShape>())
		{
			// Necessary to read world transform from the node
			SceneComponent.RootNode->UpdateTransform();

			RigidBodyComponent.RigidBody = n_new(Physics::CRigidBody(
				RigidBodyComponent.Mass, *Shape,
				RigidBodyComponent.CollisionGroupID, RigidBodyComponent.CollisionMaskID,
				SceneComponent.RootNode->GetWorldMatrix()));
			RigidBodyComponent.RigidBody->SetControlledNode(SceneComponent.RootNode);
			RigidBodyComponent.RigidBody->AttachToLevel(*pPhysicsLevel);

			// Link physics object to the entity
			RigidBodyComponent.RigidBody->UserData() = EntityID;
		}
	});

	// Setup character controllers (a special kind of physics bodies)
	World.ForEachEntityInLevelWith<CCharacterControllerComponent, CSceneComponent>(
		LevelID,
		[pPhysicsLevel](auto EntityID, auto& Entity,
			DEM::Game::CCharacterControllerComponent& Character,
			DEM::Game::CSceneComponent& SceneComponent)
	{
		DEM::Game::UpdateCharacterControllerShape(Character);

		// Necessary to read world transform from the node
		SceneComponent.RootNode->UpdateTransform();

		Character.RigidBody->SetTransform(SceneComponent.RootNode->GetWorldMatrix());
		Character.RigidBody->SetControlledNode(SceneComponent.RootNode);
		Character.RigidBody->AttachToLevel(*pPhysicsLevel);

		// Link physics object to the entity
		Character.RigidBody->UserData() = EntityID;
	});

	// Setup passive colliders
	World.ForEachComponent<CSceneComponent>([pPhysicsLevel](auto EntityID, CSceneComponent& SceneComponent)
	{
		SetupPassiveColliders(EntityID, SceneComponent, pPhysicsLevel);
	});
}
//---------------------------------------------------------------------

}
