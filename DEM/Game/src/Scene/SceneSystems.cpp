#pragma once
#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <Scene/SceneComponent.h>
#include <Physics/CollisionAttribute.h>
#include <Physics/PhysicsObject.h>
#include <Resources/ResourceManager.h>

// Scene component contains a part of scene hierarchy, including a root node
// with transform and different possible scene node attributes.

namespace DEM::Game
{

//!!!FIXME: only in active levels!
//!!!could optimize with flag-component for just created scene components! Or collect a list of entities for processing.
void InitNewSceneComponents(CGameWorld& World, Resources::CResourceManager& RsrcMgr)
{
	World.ForEachEntityWith<CSceneComponent>(
		[&World, &RsrcMgr](auto EntityID, auto& Entity, CSceneComponent& SceneComponent)
	{
		if (!SceneComponent.AssetID || (SceneComponent.RootNode && SceneComponent.RootNode->GetParent())) return;

		auto pLevel = World.FindLevel(Entity.LevelID);
		if (!pLevel) return;

		//!!!FIXME: redundant CStrID creation inside if resource exists!!!
		auto Rsrc = RsrcMgr.RegisterResource<Scene::CSceneNode>(SceneComponent.AssetID.CStr());
		auto NodeTpl = Rsrc->ValidateObject<Scene::CSceneNode>();
		if (!NodeTpl) return;

		// Instantiate asset, keeping component transform
		// FIXME: do it only if asset wasn't instantiated yet! Or clear contents on detach?
		SceneComponent.RootNode->AddChild(CStrID("asset"), NodeTpl->Clone());
		const CStrID NodeID = Entity.Name ? Entity.Name : CStrID(std::to_string(EntityID).c_str());
		pLevel->GetSceneRoot().AddChildAtPath(NodeID, SceneComponent.RootPath, SceneComponent.RootNode, true);

		// Validate resources, link collision objects to entities and node attributes
		SceneComponent.RootNode->Visit([&RsrcMgr, EntityID, pPhysicsLevel = pLevel->GetPhysics()](Scene::CSceneNode& Node)
		{
			for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			{
				Node.GetAttribute(i)->ValidateResources(RsrcMgr);

				if (auto pCollAttr = Node.GetAttribute(i)->As<Physics::CCollisionAttribute>())
				{
					pCollAttr->SetPhysicsLevel(pPhysicsLevel);
					pCollAttr->GetCollider()->UserData() = std::pair(EntityID, pCollAttr);
				}
			}
			return true;
		});
	});
}
//---------------------------------------------------------------------

void TermDeletedSceneComponents(CGameWorld& World)
{
	World.FreeDead<CSceneComponent>([&World](auto EntityID, CSceneComponent& SceneComponent)
	{
		if (SceneComponent.RootNode && SceneComponent.RootNode->GetParent())
			SceneComponent.RootNode->RemoveFromParent();
	});
}
//---------------------------------------------------------------------

}
