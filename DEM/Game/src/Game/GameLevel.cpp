#include "GameLevel.h"
#include <Game/Interaction/InteractionContext.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/LightAttribute.h>
#include <Scene/SceneNode.h>
#include <Scene/NodeAttribute.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/PhysicsObject.h>
#include <Physics/CollisionAttribute.h>
#include <Physics/BulletConv.h>
#include <AI/Navigation/NavMap.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Data/DataArray.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace DEM::Game
{
bool GetTargetFromPhysicsObject(const Physics::CPhysicsObject& Object, CTargetInfo& OutTarget);

CGameLevel::CGameLevel(CStrID ID, const Math::CAABB& Bounds, const Math::CAABB& InteractiveBounds, UPTR SubdivisionDepth)
	: _ID(ID)
	, _SceneRoot(n_new(Scene::CSceneNode(ID)))
	, _PhysicsLevel(n_new(Physics::CPhysicsLevel(Bounds)))
{
	const auto BoundsSize = Math::FromSIMD3(rtm::vector_mul(Bounds.Extent, 2.f));
	_GraphicsScene.Init(Bounds.Center, std::max({ BoundsSize.x, BoundsSize.y, BoundsSize.z }), SubdivisionDepth ? SubdivisionDepth : 12);
}
//---------------------------------------------------------------------

CGameLevel::~CGameLevel()
{
	// Order of destruction is important
	_SceneRoot = nullptr;
	_PhysicsLevel = nullptr;
}
//---------------------------------------------------------------------

PGameLevel CGameLevel::LoadFromDesc(CStrID ID, const Data::CParams& In, Resources::CResourceManager& ResMgr)
{
	vector3 Center(vector3::Zero);
	vector3 Size(512.f, 128.f, 512.f);
	int SubdivisionDepth = 0;

	In.TryGet(Center, CStrID("Center"));
	In.TryGet(Size, CStrID("Size"));
	In.TryGet(SubdivisionDepth, CStrID("SubdivisionDepth"));
	vector3 InteractiveCenter = In.Get(CStrID("InteractiveCenter"), Center);
	vector3 InteractiveExtents = In.Get(CStrID("InteractiveSize"), Size) * 0.5f;
	vector3 Extents = Size * 0.5f;

	PGameLevel Level = new CGameLevel(
		ID,
		Math::CAABB{ Math::ToSIMD(Center), Math::ToSIMD(Extents) },
		Math::CAABB{ Math::ToSIMD(InteractiveCenter), Math::ToSIMD(InteractiveExtents) },
		SubdivisionDepth);

	// Load optional scene with static graphics, collision and other attributes. No entity is associated with it.
	const bool StaticSceneIsUnique = In.Get(CStrID("StaticSceneIsUnique"), true);
	if (auto StaticScene = In.Get(CStrID("StaticScene"), Data::PParams()))
	{
		for (const auto& Param : *StaticScene)
		{
			// This resource can be unloaded by the client code when reloading it in the near future is not expected.
			// The most practical way is to check resources with refcount = 1, they are held by a resource manager only.
			// Use StaticSceneIsUnique = false if you expect to use the scene in multuple level instances and you
			// plan to modify it in the runtime (which is not recommended nor typical for _static_ scenes).
			auto Rsrc = ResMgr.RegisterResource<Scene::CSceneNode>(Param.GetValue<CString>().CStr());
			if (auto StaticSceneNode = Rsrc->ValidateObject<Scene::CSceneNode>())
			{
				// If no reuse allowed, ensure it or fall back to shared resource
				// Rsrc is referenced here and in a resource manager.
				// StaticSceneNode is referenced inside Rsrc, raw pointer is used here.
				if (StaticSceneIsUnique && Rsrc->GetRefCount() <= 2 && StaticSceneNode->GetRefCount() <= 1)
				{
					// Unregister unique scene from resources to prevent unintended reuse which can cause huge problems
					Level->GetSceneRoot().AddChild(Param.GetName(), StaticSceneNode);
					ResMgr.UnregisterResource(Rsrc->GetUID());
				}
				else
				{
					Level->GetSceneRoot().AddChild(Param.GetName(), StaticSceneNode->Clone());
				}
			}
		}
	}

	// Load navigation meshes, if present
	if (auto Navigation = In.Get(CStrID("Navigation"), Data::PDataArray()))
	{
		for (const auto& Elm : *Navigation)
		{
			const auto& NavDesc = Elm.GetValue<Data::PParams>();
			const float AgentRadius = NavDesc->Get(CStrID("AgentRadius"), -0.f);
			const float AgentHeight = NavDesc->Get(CStrID("AgentHeight"), -0.f);
			const auto NavigationMapID = NavDesc->Get(CStrID("NavMesh"), CString::Empty);
			auto Rsrc = ResMgr.RegisterResource<DEM::AI::CNavMesh>(NavigationMapID.CStr());
			if (AgentRadius <= 0.f || AgentHeight <= 0.f || !Rsrc) continue;

			if (NavDesc->Get(CStrID("Preload"), false))
				Rsrc->ValidateObject<DEM::AI::CNavMesh>();

			Level->_NavMaps.push_back(n_new(AI::CNavMap)(AgentRadius, AgentHeight, Rsrc));
		}

		std::sort(Level->_NavMaps.begin(), Level->_NavMaps.end(),
			[](const AI::PNavMap& a, const AI::PNavMap& b)
		{
			// Sort by radius, then by height ascending
			return a->GetAgentRadius() < b->GetAgentRadius() ||
				(a->GetAgentRadius() == b->GetAgentRadius() && a->GetAgentHeight() < b->GetAgentHeight());
		});
	}

	return std::move(Level);
}
//---------------------------------------------------------------------

bool CGameLevel::Validate(Resources::CResourceManager& RsrcMgr)
{
	// force entities to spawn into worlds (scene, physics etc)

	const bool Result = _SceneRoot->Visit([&RsrcMgr](Scene::CSceneNode& Node)
	{
		for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			if (!Node.GetAttribute(i)->ValidateResources(RsrcMgr)) return false;
		return true;
	});
	if (!Result) FAIL;

	if (_PhysicsLevel)
	{
		if (!_SceneRoot->Visit([PhysicsLevel = _PhysicsLevel](Scene::CSceneNode& Node)
		{
			for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
				if (auto pAttrTyped = Node.GetAttribute(i)->As<Physics::CCollisionAttribute>())
					pAttrTyped->SetPhysicsLevel(PhysicsLevel);
			OK;
		}
		)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::Update(float dt, const rtm::vector4f* pCOIArray, UPTR COICount)
{
	ZoneScoped;

	if (_PhysicsLevel) _PhysicsLevel->Update(dt);

	{
		ZoneScopedN("Scene hierarchy update");

		_SceneRoot->Update(pCOIArray, COICount);
	}

	{
		ZoneScopedN("Renderable & light transform update");

		// TODO: build some list?
		_SceneRoot->Visit([this](Scene::CSceneNode& Node)
		{
			for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
			{
				Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
				if (!Attr.IsActive()) continue;

				if (auto pAttr = Attr.As<Frame::CRenderableAttribute>())
					pAttr->UpdateInGraphicsScene(_GraphicsScene);
				else if (auto pAttr = Attr.As<Frame::CLightAttribute>())
					pAttr->UpdateInGraphicsScene(_GraphicsScene);
			}

			OK;
		});
	}
}
//---------------------------------------------------------------------

Physics::CPhysicsObject* CGameLevel::GetFirstPickIntersection(const rtm::vector4f& RayFrom, const rtm::vector4f& RayTo, rtm::vector4f* pOutPoint3D, std::string_view CollisionMask, HEntity ExcludeID) const
{
	ZoneScoped;

	if (!_PhysicsLevel) return nullptr;

	auto* pBtWorld = _PhysicsLevel->GetBtWorld();
	if (!pBtWorld) return nullptr;

	class CEntityExcludingRayCallback : public btCollisionWorld::ClosestRayResultCallback
	{
	protected:

		HEntity _ExcludeID;

	public:

		CEntityExcludingRayCallback(const btVector3& From, const btVector3& To, HEntity ExcludeID = {})
			: ClosestRayResultCallback(From, To)
			,_ExcludeID(ExcludeID)
		{
		}

		virtual bool needsCollision(btBroadphaseProxy* proxy0) const override
		{
			if (!(proxy0->m_collisionFilterGroup & m_collisionFilterMask)) return false;
			if (!(m_collisionFilterGroup & proxy0->m_collisionFilterMask)) return false;

			// TODO: if guaranteed to receive each object once, could remember if already met excluded entity and stop checking for it
			if (_ExcludeID)
			{
				if (const auto* pPhysObj = static_cast<Physics::CPhysicsObject*>(static_cast<btCollisionObject*>(proxy0->m_clientObject)->getUserPointer()))
				{
					CTargetInfo Target;
					GetTargetFromPhysicsObject(*pPhysObj, Target);
					if (Target.Entity == _ExcludeID) return false;
				}
			}

			return true;
		}
	};

	const btVector3 BtStart = Math::ToBullet3(RayFrom);
	const btVector3 BtEnd = Math::ToBullet3(RayTo);

	CEntityExcludingRayCallback RayCB(BtStart, BtEnd, ExcludeID);
	RayCB.m_collisionFilterGroup = _PhysicsLevel->PredefinedCollisionGroups.Query;
	RayCB.m_collisionFilterMask = CollisionMask.empty() ? _PhysicsLevel->PredefinedCollisionGroups.All : _PhysicsLevel->CollisionGroups.GetMask(CollisionMask);
	pBtWorld->rayTest(BtStart, BtEnd, RayCB);

	if (!RayCB.hasHit()) return nullptr;

	if (pOutPoint3D) *pOutPoint3D = Math::FromBullet(RayCB.m_hitPointWorld);
	return RayCB.m_collisionObject ? static_cast<Physics::CPhysicsObject*>(RayCB.m_collisionObject->getUserPointer()) : nullptr;
}
//---------------------------------------------------------------------

UPTR CGameLevel::EnumEntitiesInSphere(const rtm::vector4f& Position, float Radius, std::string_view CollisionMask, std::function<bool(HEntity&, const rtm::vector4f&)>&& Callback) const
{
	if (!_PhysicsLevel || !Callback) return 0;

	const auto& Groups = _PhysicsLevel->PredefinedCollisionGroups;
	const auto Group = Groups.Query;
	const auto Mask = CollisionMask.empty() ? Groups.All : _PhysicsLevel->CollisionGroups.GetMask(CollisionMask);

	//???return contact in a form of CTargetInfo? Fill CTargetInfo from physics object + bullet contact info?
	_PhysicsLevel->EnumSphereContacts(Position, Radius, Group, Mask, [&Callback](Physics::CPhysicsObject& PhysObj, const rtm::vector4f& ContactPos)
	{
		CTargetInfo Target;
		if (!GetTargetFromPhysicsObject(PhysObj, Target) || !Target.Entity) return true;

		Target.Point = ContactPos;

		return Callback(Target.Entity, ContactPos); // FIXME: return target info?
	});

	return 0;
}
//---------------------------------------------------------------------

DEM::AI::CNavMap* CGameLevel::GetNavMap(float AgentRadius, float AgentHeight) const
{
	// Navigation meshes are sorted by agent radius, then by height, so the first
	// matching navmesh is the best one as it allows the most possible movement.
	auto It = _NavMaps.begin();
	for (; It != _NavMaps.end(); ++It)
		if (AgentRadius <= (*It)->GetAgentRadius() && AgentHeight <= (*It)->GetAgentHeight())
			break;

	return (It != _NavMaps.end()) ? (*It) : nullptr;
}
//---------------------------------------------------------------------

void CGameLevel::SetNavRegionController(CStrID RegionID, HEntity Controller)
{
	for (const auto& NavMap : _NavMaps)
		NavMap->SetRegionController(RegionID, Controller);
}
//---------------------------------------------------------------------

void CGameLevel::SetNavRegionFlags(CStrID RegionID, U16 Flags, bool On)
{
	for (const auto& NavMap : _NavMaps)
		NavMap->SetRegionFlags(RegionID, Flags, On);
}
//---------------------------------------------------------------------

}

//////////////// TODO: REMOVE ///////////////////////////////
#include <Scripting/ScriptObject.h>
#include <AI/AILevel.h>

namespace Game
{

CGameLevel::CGameLevel()
{
}
//---------------------------------------------------------------------

CGameLevel::~CGameLevel()
{
	Term();
}
//---------------------------------------------------------------------

bool CGameLevel::Load(CStrID LevelID, const Data::CParams& Desc)
{
	//n_assert(!Initialized);

	/*
	ID = LevelID; //Desc.Get<CStrID>(CStrID("ID"), CStrID::Empty);
	Name = Desc.Get<CString>(CStrID("Name"), CString::Empty);

	CString PathBase("Levels:");
	PathBase += LevelID.CStr();

	// Load level script

	CString ScriptFile = PathBase + ".lua";
	if (IOSrv->FileExists(ScriptFile))
	{
		Script = n_new(Scripting::CScriptObject((CString("Level_") + ID.CStr()).CStr()));
		Script->Init(); // No special class
		if (ExecResultIsError(Script->LoadScriptFile(ScriptFile)))
			Sys::Log("Error loading script for level %s\n", ID.CStr());
	}

	// Create scene and spatial partitioning structure (always)

	vector3 SceneCenter(vector3::Zero);
	vector3 SceneExtents(512.f, 128.f, 512.f);
	int SPSHierarchyDepth = 3;

	Data::PParams SubDesc;
	if (Desc.Get(SubDesc, CStrID("Scene")))
	{
		SubDesc->Get(SceneCenter, CStrID("Center"));
		SubDesc->Get(SceneExtents, CStrID("Extents"));
		SubDesc->Get(SPSHierarchyDepth, CStrID("QuadTreeDepth"));
	}

	SceneRoot = n_new(Scene::CSceneNode(CStrID::Empty));

	//!!!Can load base scene here instead of creating empty root!
	//(loading is especially useful for baking static objects)

	SPS.Init(SceneCenter, SceneExtents * 2.f, (UPTR)SPSHierarchyDepth);

	// Create physics layer, if requested

	if (Desc.Get(SubDesc, CStrID("Physics")))
	{
		vector3 Center = SubDesc->Get(CStrID("Center"), vector3::Zero);
		vector3 Extents = SubDesc->Get(CStrID("Extents"), vector3(512.f, 128.f, 512.f));

		PhysicsLevel = n_new(Physics::CPhysicsLevel(CAABB(Center, Extents)));

		//!!!???load .bullet base contents!? useful for static collisions!
		// .bullet with non-entity data is not a resource, it is more like a level data part!
	}

	// Create AI and navigation layer, if requested

	if (Desc.Get(SubDesc, CStrID("AI")))
	{
		vector3 Center = SubDesc->Get(CStrID("Center"), vector3::Zero);
		vector3 Extents = SubDesc->Get(CStrID("Extents"), vector3(512.f, 128.f, 512.f));
		int QTDepth = SubDesc->Get<int>(CStrID("QuadTreeDepth"), 3);
		CAABB Bounds(Center, Extents);

		AILevel = n_new(AI::CAILevel);
		if (!AILevel->Init(Bounds, QTDepth)) FAIL;

		CString NMFile = PathBase + ".nm";
		if (IOSrv->FileExists(NMFile))
		{
			if (!AILevel->LoadNavMesh(NMFile))
				Sys::Log("Error loading navigation mesh for level %s\n", ID.CStr());

			//Data::PParams NavRegDesc;
			//if (SubDesc->Get(NavRegDesc, CStrID("Regions")))
			//	for (int i = 0; i < NavRegDesc->GetCount(); ++i)
			//		AILevel->SwitchNavRegionFlags(NavRegDesc->Get(i).GetName(), NavRegDesc->Get<bool>(i), NAV_FLAG_LOCKED);
		}
	}

	// Create entities, initially inactive

	if (Desc.Get(SubDesc, CStrID("Entities")))
	{
		FireEvent(CStrID("OnEntitiesLoading"));

		for (UPTR i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& EntityPrm = SubDesc->Get(i);
			if (!EntityPrm.IsA<Data::PParams>()) continue;
			Data::PParams EntityDesc = EntityPrm.GetValue<Data::PParams>();

			//!!!move to separate function to allow creating entities after level is loaded!

			const CString& TplName = EntityDesc->Get<CString>(CStrID("Tpl"), CString::Empty);
			if (TplName.IsValid())
			{
				Data::PParams Tpl = ParamsUtils::LoadParamsFromPRM("EntityTpls:" + TplName + ".prm");
				if (Tpl.IsNullPtr())
				{
					Sys::Log("Entity template '%s' not found for entity %s in level %s\n",
						TplName.CStr(), EntityPrm.GetName().CStr(), ID.CStr());
					continue;
				}
				Data::PParams MergedDesc = n_new(Data::CParams(EntityDesc->GetCount() + Tpl->GetCount()));
				Tpl->MergeDiff(*MergedDesc, *EntityDesc);
				EntityDesc = MergedDesc;
			}

			Data::CParams& RefEntityDesc = *EntityDesc.Get();

			PEntity Entity = GameSrv->GetEntityMgr()->CreateEntity(EntityPrm.GetName(), *this);
			if (Entity.IsNullPtr())
			{
				Sys::Log("Entity %s in a level %s not loaded\n", EntityPrm.GetName().CStr(), ID.CStr());
				continue;
			}

			Data::PParams AttrsDesc;
			if (RefEntityDesc.Get(AttrsDesc, CStrID("Attrs")) && AttrsDesc->GetCount())
			{
				Entity->BeginNewAttrs(AttrsDesc->GetCount());
				for (UPTR i = 0; i < AttrsDesc->GetCount(); ++i)
				{
					const Data::CParam& Attr = AttrsDesc->Get(i);
					Entity->AddNewAttr(Attr.GetName(), Attr.GetRawValue());
				}
				Entity->EndNewAttrs();
			}

			Data::PDataArray Props;
			if (RefEntityDesc.Get(Props, CStrID("Props")))
				for (UPTR i = 0; i < Props->GetCount(); ++i)
				{
					const Data::CData& PropID = Props->Get(i);
					const Core::CRTTI* pRTTI = nullptr;
					if (PropID.IsA<int>()) pRTTI = Core::CFactory::Instance().GetRTTI(static_cast<uint32_t>(PropID.GetValue<int>()));
					else if (PropID.IsA<CString>()) pRTTI = Core::CFactory::Instance().GetRTTI(PropID.GetValue<CString>());

					if (pRTTI) GameSrv->GetEntityMgr()->AttachProperty(*Entity, pRTTI);
					else Sys::Log("Failed to attach property #%d to entity %s at level %s\n", i, EntityPrm.GetName().CStr(), ID.CStr());
				}
		}

		FireEvent(CStrID("OnEntitiesLoaded"));
	}

	// Broadcast all global events to the level script and hosted entities

	GlobalSub = EventSrv->Subscribe(nullptr, this, &CGameLevel::OnEvent);

	*/

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::Term()
{
	GlobalSub.Disconnect();
	AILevel = nullptr;
	PhysicsLevel = nullptr;
	SceneRoot = nullptr;
	Script = nullptr;
}
//---------------------------------------------------------------------

bool CGameLevel::Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc)
{
	// This is a chance for all properties to write their attrs to entities
	FireEvent(CStrID("OnLevelSaving")); //, &OutDesc);

	// Save selection
	//!!!in views saving!
	//Data::PDataArray SGSelection = n_new(Data::CDataArray);
	//for (UPTR i = 0; i < SelectedEntities.GetCount(); ++i)
	//	SGSelection->Add(SelectedEntities[i]);
	//OutDesc.Set(CStrID("SelectedEntities"), SGSelection);

	// Save nav. regions status
	// No iterator, no consistency. Needs redesign.
	//if (AILevel.IsValid())
	//{
	//	Data::PParams SGAI = n_new(Data::CParams);
	//	OutDesc.Set(CStrID("AI"), SGAI);

	//	// In fact, must save per-nav-poly flags, because regions may intersect
	//	Data::PParams CurrRegionsDesc = n_new(Data::CParams);
	//	for ()

	//	Data::PParams InitialAI;
	//	Data::PParams InitialRegions;
	//	if (pInitialDesc &&
	//		pInitialDesc->Get(InitialAI, CStrID("AI")) &&
	//		InitialAI->Get(InitialRegions, CStrID("Regions")))
	//	{
	//		Data::PParams SGRegions = n_new(Data::CParams);
	//		InitialRegions->GetDiff(*SGRegions, *CurrRegionsDesc);
	//		if (SGRegions->GetCount()) SGAI->Set(CStrID("Regions"), SGRegions);
	//	}
	//	else SGAI->Set(CStrID("Regions"), CurrRegionsDesc);
	//}

	/*
	// Save entities diff
	Data::PParams SGEntities = n_new(Data::CParams);

	Data::PParams InitialEntities;
	if (pInitialDesc && pInitialDesc->Get(InitialEntities, CStrID("Entities")))
	{
		for (UPTR i = 0; i < InitialEntities->GetCount(); ++i)
		{
			CStrID EntityID = InitialEntities->Get(i).GetName();
			CEntity* pEntity = GameSrv->GetEntityMgr()->GetEntity(EntityID);
			if (!pEntity || pEntity->GetLevel() != this)
				SGEntities->Set(EntityID, Data::CData());
		}
	}

	//???is there any better way to iterate over all entities of this level? mb send them an event?
	CArray<CEntity*> Entities(128, 128);
	GameSrv->GetEntityMgr()->GetEntitiesByLevel(this, Entities);
	Data::PParams SGEntity = n_new(Data::CParams);
	const Data::CParams* pInitialEntities = InitialEntities.IsValidPtr() && InitialEntities->GetCount() ? InitialEntities.Get() : nullptr;
	for (UPTR i = 0; i < Entities.GetCount(); ++i)
	{
		CEntity* pEntity = Entities[i];
		if (SGEntity->GetCount()) SGEntity = n_new(Data::CParams);
		Data::PParams InitialDesc = pInitialEntities ? pInitialEntities->Get<Data::PParams>(pEntity->GetUID(), nullptr).Get() : nullptr;
		if (InitialDesc.IsValidPtr())
		{
			const CString& TplName = InitialDesc->Get<CString>(CStrID("Tpl"), CString::Empty);
			if (TplName.IsValid())
			{
				Data::PParams Tpl = ParamsUtils::LoadParamsFromPRM("EntityTpls:" + TplName + ".prm");
				n_assert(Tpl.IsValidPtr());
				Data::PParams MergedDesc = n_new(Data::CParams(InitialDesc->GetCount() + Tpl->GetCount()));
				Tpl->MergeDiff(*MergedDesc, *InitialDesc);
				InitialDesc = MergedDesc;
			}
		}
		pEntity->Save(*SGEntity, InitialDesc);
		if (SGEntity->GetCount()) SGEntities->Set(pEntity->GetUID(), SGEntity);
	}

	OutDesc.Set(CStrID("Entities"), SGEntities);
	*/

	OK;
}
//---------------------------------------------------------------------

//bool CGameServer::SetActiveLevel(CStrID ID)
//{
//	PGameLevel NewLevel;
//	if (ID.IsValid())
//	{
//		IPTR LevelIdx = Levels.FindIndex(ID);
//		if (LevelIdx == INVALID_INDEX) FAIL;
//		NewLevel = Levels.ValueAt(LevelIdx);
//	}
//
//	if (NewLevel != ActiveLevel)
//	{
//		EventSrv->FireEvent(CStrID("OnActiveLevelChanging"));
//		ActiveLevel = NewLevel;
//		SetGlobalAttr<CStrID>(CStrID("ActiveLevel"), ActiveLevel.IsValidPtr() ? ID : CStrID::Empty);
//
//		EntityUnderMouse = CStrID::Empty;
//		HasMouseIsect = false;
//		UpdateMouseIntersectionInfo();
//
//		EventSrv->FireEvent(CStrID("OnActiveLevelChanged"));
//	}
//
//	OK;
//}
////---------------------------------------------------------------------

//void CGameLevel::RenderScene()
//{
////???add to each view variables?
//	RenderSrv->SetAmbientLight(AmbientLight);
//	RenderSrv->SetCameraPosition(MainCamera->GetPosition());
//	RenderSrv->SetViewProjection(ViewProj);
//
// Dependent cameras:
	// Some shapes may request textures that are RTs of specific cameras
	// These textures must be rendered before shapes are rendered
	// Good way is to collect all cameras and recurse, filling required textures
	// Pass may disable recursing into cameras to prevent infinite recursion
	// Generally, only FrameBuffer pass should recurse
	// Camera shouldn't render its texture more than once per frame (it won't change)!
	//???as "RenderDependentCameras" flag in Camera/Pass? maybe even filter by dependent camera type
	// Collect - check all meshes to be rendered, check all their textures, select textures rendered from camera RTs
	// Non-mesh dependent textures (shadows etc) must be rendered in one of the previous passes

	// Shapes are sorted by shader, by distance (None, FtB or BtF, depending on shader requirements),
	// by geometry (for the instancing), may be by lights that affect them
	// For the front-to-back sorting, can sort once on first request, always FtB, and when BtF is needed,
	// iterate through the array from the end reversely

	// For each shader (batch):
	// Shader is set
	// Light params are updated, if it is light pass
	// Shader params are updated
	// Shapes are rendered, instanced, when possible

//!!!NOTE: meshes visible from the light's camera are the meshes in light's range!
// can avoid collecting visible meshes on shadow pass
// can render non-shadow-receiving objects without shadow mapping (another technique)
// can render to SM only casters with extruded shadow box visible from main camera

// Rendering must be like this:
	// - begin frame shader
	// - Renderer: apply frame shader constant render states
	// - Renderer: set View and Projection
	// - determine visible meshes [and lights, if lighting is enabled ?in one of passes?]
	// - for each pass, render scene pass, occlusion pass, shadow pass (for shadow-casting lights) or posteffect pass
	// - end frame shader
	//
	// After this some UI, text and debug shapes can be rendered
	// Nebula treats all them as different batches or render plugins, it is good idea maybe...
	// Then backbuffer is present
	//
	// Scene pass:
	// - begin pass
	// - if pass renders dependent textures
	//   - collect them and their cameras (check if already up-to-date this frame)
	//   - if recurse to rendering call with another camera and frame shader
	//   - now our textures from cameras are ready to use
	// - Renderer: set RT
	// - Renderer: optionally clear RT
	// - Renderer: apply pass shader (divide this pass to some phases/batches like opaque, atest, alpha etc inside, mb before a renderer...)
	// - Renderer: set pass technique
	// - Renderer: pass meshes [and lights]
	// - Renderer: render to RT
	//    (link meshes and lights(here?), sort meshes, batch instances, select lighting code,
	//     set shared state of instance sets)
	// - end pass
//}
////---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset) const
{
	//Frame::PCameraAttribute MainCamera; //!!!DBG TMP!
	//if (MainCamera.IsNullPtr()) FAIL;
	//vector3 EntityPos = Entity.GetAttr<matrix44>(CStrID("Transform")).Translation();
	//if (Offset) EntityPos += *Offset;
	//MainCamera->GetPoint2D(EntityPos, Out.x, Out.y);
	OK;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const
{
	//Frame::PCameraAttribute MainCamera; //!!!DBG TMP!
	//if (MainCamera.IsNullPtr()) FAIL;

	//Prop::CPropSceneNode* pNode = Entity.GetProperty<Prop::CPropSceneNode>();
	//if (!pNode) FAIL;

	//CAABB AABB;
	//pNode->GetAABB(AABB);
	//vector3 Center = AABB.Center();
	//MainCamera->GetPoint2D(vector3(Center.x, AABB.Max.y, Center.z), Out.x, Out.y);
	OK;
}
//---------------------------------------------------------------------

}
