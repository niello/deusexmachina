#include "GameWorld.h"
#include <Game/GameLevel.h>
#include <AI/AILevel.h>
#include <Scene/SceneNode.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>

namespace DEM::Game
{

CGameWorld::CGameWorld(Resources::CResourceManager& ResMgr)
	: _ResMgr(ResMgr)
{
	//???register default loader for entity template?
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

CGameLevel* CGameWorld::LoadLevel(CStrID ID, const Data::CParams& BaseData, ISaveLoadDelegate* pStateLoader)
{
	// If level exists, must unload it or fail an operation
	// Unloading level ensures there are no its entities left

	vector3 Center(vector3::Zero);
	vector3 Size(512.f, 128.f, 512.f);
	int SubdivisionDepth = 0;

	BaseData.Get(Center, CStrID("Center"));
	BaseData.Get(Size, CStrID("Size"));
	BaseData.Get(SubdivisionDepth, CStrID("SubdivisionDepth"));
	vector3 InteractiveCenter = BaseData.Get(CStrID("InteractiveCenter"), Center);
	vector3 InteractiveSize = BaseData.Get(CStrID("InteractiveSize"), Size);

	auto Level = n_new(DEM::Game::CGameLevel(ID, CAABB(Center, Size * 0.5f), CAABB(InteractiveCenter, InteractiveSize * 0.5f), SubdivisionDepth));

	// Load optional scene with static graphics, collision and other attributes. No entity is associated with it.
	auto StaticSceneID = BaseData.Get(CStrID("StaticScene"), CString::Empty);
	if (!StaticSceneID.IsEmpty())
	{
		// This resource can be unloaded by the client code when reloading it in the near future is not expected.
		// The most practical way is to check resources with refcount = 1, they are held by a resource manager only.
		// Use StaticSceneIsUnique = false if you expect to use the scene in multuple level instances and you
		// plan to modify it in the runtime (which is not recommended nor typical for _static_ scenes).
		const bool IsUnique = BaseData.Get(CStrID("StaticSceneIsUnique"), true);
		auto Rsrc = _ResMgr.RegisterResource<Scene::CSceneNode>(CStrID(StaticSceneID.CStr()));
		if (auto StaticSceneNode = Rsrc->ValidateObject<Scene::CSceneNode>())
			Level->GetSceneRoot().AddChild(CStrID("StaticScene"), IsUnique ? *StaticSceneNode : *StaticSceneNode->Clone());
	}

	// Load navigation map, if present
	auto NavigationMapID = BaseData.Get(CStrID("NavigationMap"), CString::Empty);
	if (!NavigationMapID.IsEmpty() && Level->GetAI())
	{
		n_verify(Level->GetAI()->LoadNavMesh(NavigationMapID.CStr()));
	}

	//???temporary entity template cache? or store all templates? or some data about what tpls are needed at runtime?
	//???entity template as a resource? Almost the same as a .scn template!

	// list of entities loaded by loader (reserve 256 records), write erased there too
	// if (pStateLoader) pStateLoader->LoadEntities(*this, level(id?), BaseData.EntitiesSection);

	//???if !pStateLoader || pStateLoader->IsDiff/NeedLoadBase?
	//Plus can compare entity count in list of loaded entities and in a base
	// for any entity in BaseData
	//   if entity was loaded by loader, skip
	//   load entity with templates

	// For each entity
	//   Create entity
	//   Create components
	//   Load values to components
	//   Attach entity to the level

	// Notify level loaded / activated, validate if automatic

	// Return level ptr

	return nullptr;
}
//---------------------------------------------------------------------

}