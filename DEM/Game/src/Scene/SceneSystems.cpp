#pragma once
#include <Game/ECS/GameWorld.h>
#include <Game/GameLevel.h>
#include <Scene/SceneComponent.h>
#include <Scene/NodeAttribute.h>
#include <Resources/ResourceManager.h>

namespace DEM::Game
{

void InitNewSceneComponent(HEntity EntityID, const CEntity& Entity, CSceneComponent& SceneComponent, CGameWorld& World, Resources::CResourceManager& RsrcMgr)
{
	if (!SceneComponent.AssetID || !SceneComponent.RootNode) return;

	// Instantiate asset, keeping component transform
	//!!!FIXME: redundant CStrID creation!!!
	//???need entity ID added to the node name? to attach multiple scene components to one node
	const CStrID sidAsset("asset");
	if (!SceneComponent.RootNode->GetChild(sidAsset))
	{
		//!!!FIXME: redundant CStrID creation inside if resource exists!!!
		auto Rsrc = RsrcMgr.RegisterResource<Scene::CSceneNode>(SceneComponent.AssetID.CStr());
		auto NodeTpl = Rsrc->ValidateObject<Scene::CSceneNode>();
		if (!NodeTpl) return;

		SceneComponent.RootNode->AddChild(sidAsset, NodeTpl->Clone());
	}

	// Attach our node to the parent if not yet
	if (!SceneComponent.RootNode->GetParent())
	{
		auto pLevel = World.FindLevel(Entity.LevelID);
		if (!pLevel) return;
		const CStrID NodeID = Entity.Name ? Entity.Name : CStrID(std::to_string(EntityID).c_str());
		pLevel->GetSceneRoot().AddChildAtPath(NodeID, SceneComponent.RootPath, SceneComponent.RootNode, true);
	}

	// Validate resources
	SceneComponent.RootNode->Visit([&RsrcMgr](Scene::CSceneNode& Node)
	{
		for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			Node.GetAttribute(i)->ValidateResources(RsrcMgr);
		return true;
	});
}
//---------------------------------------------------------------------

//!!!FIXME: only in active levels!
void InitNewSceneComponents(CGameWorld& World, Resources::CResourceManager& RsrcMgr)
{
	World.ForEachEntityWith<CSceneComponent>(
		[&World, &RsrcMgr](auto EntityID, auto& Entity, CSceneComponent& SceneComponent)
	{
		InitNewSceneComponent(EntityID, Entity, SceneComponent, World, RsrcMgr);
	});
}
//---------------------------------------------------------------------

}
