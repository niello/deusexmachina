#include "GameLevel.h"

#include <Game/Entity.h>
#include <Game/GameServer.h>
#include <Scripting/ScriptObject.h>
#include <Render/FrameShader.h>
#include <Render/SceneNodeUpdateInSPS.h>
#include <Scene/SceneNodeRenderDebug.h>
#include <Scene/PropSceneNode.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsServer.h>
#include <AI/AILevel.h>
#include <Events/EventServer.h>
#include <IO/IOServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Game
{

CGameLevel::~CGameLevel()
{
	Term();
}
//---------------------------------------------------------------------

void PhysicsPreTick(btDynamicsWorld* world, btScalar timeStep)
{
	n_assert_dbg(world && world->getWorldUserInfo());
	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("FrameTime"), (float)timeStep);
	((CGameLevel*)world->getWorldUserInfo())->FireEvent(CStrID("BeforePhysicsTick"), P);
}
//---------------------------------------------------------------------

void PhysicsTick(btDynamicsWorld* world, btScalar timeStep)
{
	n_assert_dbg(world && world->getWorldUserInfo());
	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("FrameTime"), (float)timeStep);
	((CGameLevel*)world->getWorldUserInfo())->FireEvent(CStrID("AfterPhysicsTick"), P);

	/*for (int i = 0; i < world->getDispatcher()->getNumManifolds(); ++i)
	{
		btPersistentManifold* pManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
		//pManifold->getBody0();
	}*/
}
//---------------------------------------------------------------------

bool CGameLevel::Init(CStrID LevelID, const Data::CParams& Desc)
{
	//n_assert(!Initialized);

	ID = LevelID; //Desc.Get<CStrID>(CStrID("ID"), CStrID::Empty);
	Name = Desc.Get<CString>(CStrID("Name"), NULL);

	CString PathBase = "Levels:";
	PathBase += LevelID.CStr();

	CString ScriptFile = PathBase + ".lua";
	if (IOSrv->FileExists(ScriptFile))
	{
		Script = n_new(Scripting::CScriptObject((CString("Level_") + ID.CStr()).CStr()));
		Script->Init(); // No special class
		if (ExecResultIsError(Script->LoadScriptFile(ScriptFile)))
			Sys::Log("Error loading script for level %s\n", ID.CStr());
	}

	Data::PParams SubDesc;
	if (Desc.Get(SubDesc, CStrID("Scene")))
	{
		SceneRoot = n_new(Scene::CSceneNode(CStrID::Empty));

		vector3 Center = SubDesc->Get(CStrID("Center"), vector3::Zero);
		vector3 Extents = SubDesc->Get(CStrID("Extents"), vector3(512.f, 128.f, 512.f));
		BBox.Set(Center, Extents);

		int SPSHierarchyDepth = SubDesc->Get<int>(CStrID("QuadTreeDepth"), 3);

		SPS.Build(Center.x, Center.z, Extents.x * 2.f, Extents.z * 2.f, (uchar)SPSHierarchyDepth);

		CameraManager = n_new(Scene::CCameraManager);

		////!!!create in level. no default camera. fail to render view without camera being set! LODs use camera!
		//???!!!if CameraManager is necessary, add there method CreateMainCamera() or smth like that and just call it here!
		//!!!or do it in its constructor!
		//CSceneNode* CameraNode = RootNode->CreateChild(CStrID("_DefaultCamera"));
		//MainCamera = n_new(CCamera);
		//CameraNode->AddAttr(*MainCamera);
		//MainCamera->SetWidth((float)RenderSrv->GetBackBufferWidth());
		//MainCamera->SetHeight((float)RenderSrv->GetBackBufferHeight());

	//void		SetMainCamera(CCamera* pNewCamera);
	//CCamera&	GetMainCamera() const { return *MainCamera; }
//void CScene::SetMainCamera(CCamera* pNewCamera)
//{
//	MainCamera = pNewCamera;
//	if (pNewCamera && AutoAdjustCameraAspect)
//	{
//		MainCamera->SetWidth((float)RenderSrv->GetBackBufferWidth());
//		MainCamera->SetHeight((float)RenderSrv->GetBackBufferHeight());
//	}
//}

		Data::PParams CameraDesc;
		if (SubDesc->Get(CameraDesc, CStrID("Camera")))
		{
			bool IsThirdPerson = CameraDesc->Get(CStrID("ThirdPerson"), true);
			n_assert(IsThirdPerson); // Until a first person camera is implemented

			if (IsThirdPerson)
			{
				CameraManager->InitThirdPersonCamera(*MainCamera->GetNode());
				Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)CameraManager->GetCameraController();
				if (pCtlr)
				{
					pCtlr->SetVerticalAngleLimits(n_deg2rad(CameraDesc->Get(CStrID("MinVAngle"), 0.0f)), n_deg2rad(CameraDesc->Get(CStrID("MaxVAngle"), 89.999f)));
					pCtlr->SetDistanceLimits(CameraDesc->Get(CStrID("MinDistance"), 0.0f), CameraDesc->Get(CStrID("MaxDistance"), 10000.0f));
					pCtlr->SetCOI(CameraDesc->Get(CStrID("COI"), vector3::Zero));
					pCtlr->SetAngles(n_deg2rad(CameraDesc->Get(CStrID("VAngle"), 0.0f)), n_deg2rad(CameraDesc->Get(CStrID("HAngle"), 0.0f)));
					pCtlr->SetDistance(CameraDesc->Get(CStrID("Distance"), 20.0f));
				}
			}
		}
	}

	if (Desc.Get(SubDesc, CStrID("Physics")))
	{
		vector3 Center = SubDesc->Get(CStrID("Center"), vector3::Zero);
		vector3 Extents = SubDesc->Get(CStrID("Extents"), vector3(512.f, 128.f, 512.f));
		CAABB Bounds(Center, Extents);

		PhysWorld = n_new(Physics::CPhysicsWorld);
		if (!PhysWorld->Init(Bounds)) FAIL;

		PhysWorld->GetBtWorld()->setInternalTickCallback(PhysicsPreTick, this, true);
		PhysWorld->GetBtWorld()->setInternalTickCallback(PhysicsTick, this, false);
	}

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

	EventSrv->Subscribe(NULL, this, &CGameLevel::OnEvent, &GlobalSub);

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::Term()
{
	GlobalSub = NULL;
	CameraManager = NULL;
	AILevel = NULL;
	PhysWorld = NULL;
	SceneRoot = NULL;
	Script = NULL;
}
//---------------------------------------------------------------------

bool CGameLevel::Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc)
{
	// This is a chance for all properties to write their attrs to entities
	FireEvent(CStrID("OnLevelSaving")); //, &OutDesc);

	// Save selection
	Data::PDataArray SGSelection = n_new(Data::CDataArray);
	for (int i = 0; i < SelectedEntities.GetCount(); ++i)
		SGSelection->Add(SelectedEntities[i]);
	OutDesc.Set(CStrID("SelectedEntities"), SGSelection);

	// Save camera state
	if (CameraManager.IsValid())
	{
		Data::PParams SGScene = n_new(Data::CParams);

		bool IsThirdPerson = CameraManager->IsCameraThirdPerson();
		n_assert(IsThirdPerson); // Until a first person camera is implemented

		Data::PParams CurrCameraDesc = n_new(Data::CParams);
		CurrCameraDesc->Set(CStrID("ThirdPerson"), IsThirdPerson);

		if (IsThirdPerson)
		{
			Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)CameraManager->GetCameraController();
			if (pCtlr)
			{
				CurrCameraDesc->Set(CStrID("MinVAngle"), n_rad2deg(pCtlr->GetVerticalAngleMin()));
				CurrCameraDesc->Set(CStrID("MaxVAngle"), n_rad2deg(pCtlr->GetVerticalAngleMax()));
				CurrCameraDesc->Set(CStrID("MinDistance"), pCtlr->GetDistanceMin());
				CurrCameraDesc->Set(CStrID("MaxDistance"), pCtlr->GetDistanceMax());
				CurrCameraDesc->Set(CStrID("COI"), pCtlr->GetCOI());
				CurrCameraDesc->Set(CStrID("HAngle"), n_rad2deg(pCtlr->GetAngles().Phi));
				CurrCameraDesc->Set(CStrID("VAngle"), n_rad2deg(pCtlr->GetAngles().Theta));
				CurrCameraDesc->Set(CStrID("Distance"), pCtlr->GetDistance());
			}
		}

		Data::PParams InitialScene;
		Data::PParams InitialCamera;
		if (pInitialDesc &&
			pInitialDesc->Get(InitialScene, CStrID("Scene")) &&
			InitialScene->Get(InitialCamera, CStrID("Camera")))
		{
			Data::PParams SGCamera = n_new(Data::CParams);
			InitialCamera->GetDiff(*SGCamera, *CurrCameraDesc);
			if (SGCamera->GetCount()) SGScene->Set(CStrID("Camera"), SGCamera);
		}
		else SGScene->Set(CStrID("Camera"), CurrCameraDesc);

		if (SGScene->GetCount()) OutDesc.Set(CStrID("Scene"), SGScene);
	}

	// Save nav. regions status
	// No iterator, no consistency. Needs redesign.
/*	if (AILevel.IsValid())
	{
		Data::PParams SGAI = n_new(Data::CParams);
		OutDesc.Set(CStrID("AI"), SGAI);

		// In fact, must save per-nav-poly flags, because regions may intersect
		Data::PParams CurrRegionsDesc = n_new(Data::CParams);
		for ()

		Data::PParams InitialAI;
		Data::PParams InitialRegions;
		if (pInitialDesc &&
			pInitialDesc->Get(InitialAI, CStrID("AI")) &&
			InitialAI->Get(InitialRegions, CStrID("Regions")))
		{
			Data::PParams SGRegions = n_new(Data::CParams);
			InitialRegions->GetDiff(*SGRegions, *CurrRegionsDesc);
			if (SGRegions->GetCount()) SGAI->Set(CStrID("Regions"), SGRegions);
		}
		else SGAI->Set(CStrID("Regions"), CurrRegionsDesc);
	}
*/

	// Save entities diff
	Data::PParams SGEntities = n_new(Data::CParams);

	Data::PParams InitialEntities;
	if (pInitialDesc && pInitialDesc->Get(InitialEntities, CStrID("Entities")))
	{
		for (int i = 0; i < InitialEntities->GetCount(); ++i)
		{
			CStrID EntityID = InitialEntities->Get(i).GetName();
			CEntity* pEntity = EntityMgr->GetEntity(EntityID, false);
			if (!pEntity || pEntity->GetLevel() != this)
			{
				// Static objects never change, so we need no diff of them
				CStaticObject* pStaticObj = StaticEnvMgr->GetStaticObject(EntityID);
				if (!pStaticObj || &pStaticObj->GetLevel() != this)
					SGEntities->Set(EntityID, Data::CData());
			}
		}
	}

	//???is there any better way to iterate over all entities of this level? mb send them an event?
	CArray<CEntity*> Entities(128, 128);
	EntityMgr->GetEntitiesByLevel(this, Entities);
	Data::PParams SGEntity = n_new(Data::CParams);
	const Data::CParams* pInitialEntities = InitialEntities.IsValid() && InitialEntities->GetCount() ? InitialEntities.GetUnsafe() : NULL;
	for (int i = 0; i < Entities.GetCount(); ++i)
	{
		CEntity* pEntity = Entities[i];
		if (SGEntity->GetCount()) SGEntity = n_new(Data::CParams);
		Data::PParams InitialDesc = pInitialEntities ? pInitialEntities->Get<Data::PParams>(pEntity->GetUID(), NULL).GetUnsafe() : NULL;
		if (InitialDesc.IsValid())
		{
			const CString& TplName = InitialDesc->Get<CString>(CStrID("Tpl"), CString::Empty);
			if (TplName.IsValid())
			{
				Data::PParams Tpl = DataSrv->LoadPRM("EntityTpls:" + TplName + ".prm");
				n_assert(Tpl.IsValid());
				Data::PParams MergedDesc = n_new(Data::CParams(InitialDesc->GetCount() + Tpl->GetCount()));
				Tpl->MergeDiff(*MergedDesc, *InitialDesc);
				InitialDesc = MergedDesc;
			}
		}
		pEntity->Save(*SGEntity, InitialDesc);
		if (SGEntity->GetCount()) SGEntities->Set(pEntity->GetUID(), SGEntity);
	}

	OutDesc.Set(CStrID("Entities"), SGEntities);

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::Trigger()
{
	FireEvent(CStrID("BeforeTransforms"));

	if (SceneRoot.IsValid()) SceneRoot->UpdateLocalSpace();

	if (PhysWorld.IsValid())
	{
		FireEvent(CStrID("BeforePhysics"));
		PhysWorld->Trigger((float)GameSrv->GetFrameTime());
		FireEvent(CStrID("AfterPhysics"));
	}

	if (SceneRoot.IsValid()) SceneRoot->UpdateWorldSpace();

	FireEvent(CStrID("AfterTransforms"));
}
//---------------------------------------------------------------------

bool CGameLevel::OnEvent(const Events::CEventBase& Event)
{
	CStrID EvID = ((Events::CEvent&)Event).ID;

	if (EvID == CStrID("OnBeginFrame")) ProcessPendingEvents();
	else if (AutoAdjustCameraAspect && MainCamera.IsValid() && EvID == CStrID("OnRenderDeviceReset"))
	{
		MainCamera->SetWidth((float)RenderSrv->GetBackBufferWidth());
		MainCamera->SetHeight((float)RenderSrv->GetBackBufferHeight());
	}

	return !!DispatchEvent(Event);
}
//---------------------------------------------------------------------

void CGameLevel::RenderScene()
{
	Render::PFrameShader ScreenFrameShader = RenderSrv->GetScreenFrameShader();
	if (!ScreenFrameShader.IsValid()) return;

	CArray<Render::CRenderObject*>	VisibleObjects;	//PERF: //???use buckets instead? may be it will be faster
	CArray<Render::CLight*>			VisibleLights;

	Render::CSceneNodeUpdateInSPS Visitor;
	//!!!fill visitor fields!
	if (SceneRoot.IsValid()) Visitor.Visit(*SceneRoot);

	/*
	PCamera Camera = MainCamera; //???!!!if camera manager is useful, get from it?!
	if (!Camera.IsValid())
	{
		ClearVisibleLists();
		FAIL;
	}

	//!!!FrameShader OPTIONS!
	bool FrameShaderUsesLights = true;
	//!!!filters (ShadowCasters etc)!

	const matrix44& ViewProj = Camera->GetViewProjMatrix();

	//!!!filter flags (from frame shader - or-sum of pass flags, each pass will check requirements inside itself)
	CArray<CLight*>* pVisibleLights = FrameShaderUsesLights ? &VisibleLights : NULL;
	SPSCollectVisibleObjects(SPS.GetRootNode(), ViewProj, BBox, &VisibleObjects, pVisibleLights);

	RenderSrv->SetAmbientLight(AmbientLight);
	RenderSrv->SetCameraPosition(Camera->GetPosition());
	RenderSrv->SetViewProjection(ViewProj);

	FrameShader.Render(&VisibleObjects, pVisibleLights);

// Dependent cameras:
	// Some shapes may request textures that are RTs of specific cameras
	// These textures must be rendered before shapes are rendered
	// Good way is to collect all cameras and recurse, filling required textures
	// Pass may disable recursing into cameras to prevent infinite recursion
	// Generally, only FrameBuffer pass should recurse
	// Camera shouldn't render its texture more than once per frame (in won't change)!
	//???as "RenderDependentCameras" flag in Camera/Pass? maybe even filter by dependent camera type
	// Collect - check all meshes to be rendered, check all their textures, select textures rendered from camera RTs
	// Non-mesh dependent textures (shadows etc) must be rendered in one of the previous passes

// Somewhere at here scene ends and renderer begins:

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
	//
	// Occlusion pass:
	// - begin pass
	// - issue occlusion query for AABBs of all visible meshes
	// - remove occluded shapes [and lights] from visible shape [and light] array[s]
	// - end pass
	//
	// Shadow pass:
	// - begin pass
	// - select shadow-casting light(s)
	// - instead of collecting meshes visible from the light's camera, we collect ones in the light range
	// - for directional lights and PSSM this process will differ, may be extruded shadow boxes will be necessary
	// - collect ONLY shadow casters
	// - do not modify the main mesh list
	// - note: some caster meshes invisible from the main camera can have visible shadows. Anyway they are
	//         casting shadows on all visible receivers as their shadows are rendered to shadow buffer texture
	// - render casters to the shadow buffer texture
	// - end pass
	//
	// Posteffect path:
	// - begin pass
	// - Renderer: set posteffect shader
	// - Renderer: set source textures and other params
	// - Renderer: render fullscreen quad
	// - end pass
	//
	//
	// After this some UI, text and debug shapes can be rendered
	// Nebula treats all them as different batches or render plugins, it is good idea maybe...
	// Then backbuffer is present

	ClearVisibleLists();

	OK;
	*/
}
//---------------------------------------------------------------------

//!!!???separate? or with bool flags?
void CGameLevel::RenderDebug()
{
	PhysWorld->RenderDebug();

	FireEvent(CStrID("OnRenderDebug"));

	if (SceneRoot.IsValid())
	{
		Scene::CSceneNodeRenderDebug RD;
		RD.Visit(*SceneRoot);
	}
}
//---------------------------------------------------------------------

//???write 2 versions, physics-based and mesh-based?
bool CGameLevel::GetIntersectionAtScreenPos(float XRel, float YRel, vector3* pOutPoint3D, CStrID* pOutEntityUID) const
{
	if (!Scene.IsValid() || !PhysWorld.IsValid()) FAIL;

	line3 Ray;
	Scene->GetMainCamera().GetRay3D(XRel, YRel, 5000.f, Ray); //???ray length to far plane or infinite?

	ushort Group = PhysicsSrv->CollisionGroups.GetMask("MousePick");
	ushort Mask = PhysicsSrv->CollisionGroups.GetMask("All|MousePickTarget");
	Physics::PPhysicsObj PhysObj;
	if (!PhysWorld->GetClosestRayContact(Ray.Start, Ray.End(), Group, Mask, pOutPoint3D, &PhysObj)) FAIL;

	if (pOutEntityUID)
	{
		void* pUserData = PhysObj.IsValid() ? PhysObj->GetUserData() : NULL;
		*pOutEntityUID = pUserData ? *(CStrID*)&pUserData : CStrID::Empty;
	}

	OK;
}
//---------------------------------------------------------------------

DWORD CGameLevel::GetEntitiesAtScreenRect(CArray<CEntity*>& Out, const rectangle& RelRect) const
{
	// calc frustum
	// query scene quadtree with this frustum
	// select only render objects
	// return newly selected obj count
	Sys::Error("CGameLevel::GetEntitiesAtScreenRect() -> IMPLEMENT ME!");
	return 0;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset) const
{
	if (!Scene.IsValid()) FAIL;
	vector3 EntityPos = Entity.GetAttr<matrix44>(CStrID("Transform")).Translation();
	if (Offset) EntityPos += *Offset;
	Scene->GetMainCamera().GetPoint2D(EntityPos, Out.x, Out.y);
	OK;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const
{
	if (!Scene.IsValid()) FAIL;

	Prop::CPropSceneNode* pNode = Entity.GetProperty<Prop::CPropSceneNode>();
	if (!pNode) FAIL;

	CAABB AABB;
	pNode->GetAABB(AABB);
	vector3 Center = AABB.Center();
	Scene->GetMainCamera().GetPoint2D(vector3(Center.x, AABB.Max.y, Center.z), Out.x, Out.y);
	OK;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenRect(rectangle& Out, const Game::CEntity& Entity, const vector3* Offset) const
{
	if (!Scene.IsValid()) FAIL;

	Prop::CPropSceneNode* pNode = Entity.GetProperty<Prop::CPropSceneNode>();
	if (!pNode)
	{
		matrix44 Tfm;
		if (!Entity.GetAttr(Tfm, CStrID("Transform"))) FAIL;
		Scene->GetMainCamera().GetPoint2D(Tfm.Translation(), Out.v0.x, Out.v0.y);
		Out.v1 = Out.v0;
		OK;
	}

	CAABB AABB;
	pNode->GetAABB(AABB);

	if (Offset)
	{
		AABB.Max += *Offset;
		AABB.Min += *Offset;
	}

	Scene->GetMainCamera().GetPoint2D(AABB.GetCorner(0), Out.v0.x, Out.v0.y);
	Out.v1 = Out.v0;

	vector2 ScreenPos;
	for (DWORD i = 1; i < 8; i++)
	{
		Scene->GetMainCamera().GetPoint2D(AABB.GetCorner(i), ScreenPos.x, ScreenPos.y);

		if (ScreenPos.x < Out.v0.x) Out.v0.x = ScreenPos.x;
		else if (ScreenPos.x > Out.v1.x) Out.v1.x = ScreenPos.x;

		if (ScreenPos.y < Out.v0.y) Out.v0.y = ScreenPos.y;
		else if (ScreenPos.y > Out.v1.y) Out.v1.y = ScreenPos.y;
	}

	OK;
}
//---------------------------------------------------------------------

DWORD CGameLevel::GetEntitiesInPhysBox(CArray<CEntity*>& Out, const matrix44& OBB) const
{
	// request physics level for shapes and bodies
	// select ones that are attached to entities
	// return newly selected obj count
	Sys::Error("CGameLevel::GetEntitiesInPhysBox() -> IMPLEMENT ME!");
	return 0;
}
//---------------------------------------------------------------------

DWORD CGameLevel::GetEntitiesInPhysSphere(CArray<CEntity*>& Out, const vector3& Center, float Radius) const
{
	// request physics level for shapes and bodies
	// select ones that are attached to entities
	// return newly selected obj count
	Sys::Error("CGameLevel::GetEntitiesInPhysBox() -> IMPLEMENT ME!");
	return 0;
}
//---------------------------------------------------------------------

bool CGameLevel::GetSurfaceInfoBelow(CSurfaceInfo& Out, const vector3& Position, float ProbeLength) const
{
	n_assert(ProbeLength > 0);
	vector3 Dir(0.0f, -ProbeLength, 0.0f);

	//!!!can request closest contacts for Default and Terrain!
	ushort Group = PhysicsSrv->CollisionGroups.GetMask("Default");
	ushort Mask = PhysicsSrv->CollisionGroups.GetMask("All");
	vector3 ContactPos;
	if (!PhysWorld->GetClosestRayContact(Position, Position + Dir, Group, Mask, &ContactPos)) FAIL;
	Out.WorldHeight = ContactPos.y;

	//!!!material from CPhysicsObj!

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::AddToSelection(CStrID EntityID)
{
	if (IsSelected(EntityID) || !EntityID.IsValid()) return;
	CEntity* pEnt = EntityMgr->GetEntity(EntityID);
	if (pEnt && pEnt->GetLevel() == this) //???only if IsActive?
	{
		SelectedEntities.Add(EntityID);
		Data::PParams P = n_new(Data::CParams(1));
		P->Set(CStrID("EntityID"), EntityID);
		FireEvent(CStrID("OnEntitySelected"), P);
	}
}
//---------------------------------------------------------------------

bool CGameLevel::RemoveFromSelection(CStrID EntityID)
{
	if (!SelectedEntities.RemoveByValue(EntityID)) FAIL; 
	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("EntityID"), EntityID);
	FireEvent(CStrID("OnEntityDeselected"), P);
	OK;
}
//---------------------------------------------------------------------

}