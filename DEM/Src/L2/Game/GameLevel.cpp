#include "GameLevel.h"

#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Game/EntityManager.h>
#include <Game/StaticEnvManager.h>
#include <Game/Entity.h>
#include <Game/StaticObject.h>
#include <Scripting/ScriptObject.h>
#include <Scene/SceneNodeRenderDebug.h>
#include <Scene/PropSceneNode.h>
#include <Physics/PhysicsLevel.h>
#include <Physics/PhysicsServer.h>
#include <AI/AILevel.h>
#include <Events/EventServer.h>
#include <IO/IOServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>

namespace Game
{

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
	Name = Desc.Get<CString>(CStrID("Name"), CString::Empty);

	CString PathBase("Levels:");
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

		SPS.Init(Center, Extents * 2.f, (U8)SPSHierarchyDepth);
	}

	if (Desc.Get(SubDesc, CStrID("Physics")))
	{
		vector3 Center = SubDesc->Get(CStrID("Center"), vector3::Zero);
		vector3 Extents = SubDesc->Get(CStrID("Extents"), vector3(512.f, 128.f, 512.f));
		CAABB Bounds(Center, Extents);

		PhysicsLevel = n_new(Physics::CPhysicsLevel);
		if (!PhysicsLevel->Init(Bounds)) FAIL;

		PhysicsLevel->GetBtWorld()->setInternalTickCallback(PhysicsPreTick, this, true);
		PhysicsLevel->GetBtWorld()->setInternalTickCallback(PhysicsTick, this, false);
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
	AILevel = NULL;
	PhysicsLevel = NULL;
	SceneRoot = NULL;
	Script = NULL;
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
		for (UPTR i = 0; i < InitialEntities->GetCount(); ++i)
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
	const Data::CParams* pInitialEntities = InitialEntities.IsValidPtr() && InitialEntities->GetCount() ? InitialEntities.GetUnsafe() : NULL;
	for (UPTR i = 0; i < Entities.GetCount(); ++i)
	{
		CEntity* pEntity = Entities[i];
		if (SGEntity->GetCount()) SGEntity = n_new(Data::CParams);
		Data::PParams InitialDesc = pInitialEntities ? pInitialEntities->Get<Data::PParams>(pEntity->GetUID(), NULL).GetUnsafe() : NULL;
		if (InitialDesc.IsValidPtr())
		{
			const CString& TplName = InitialDesc->Get<CString>(CStrID("Tpl"), CString::Empty);
			if (TplName.IsValid())
			{
				Data::PParams Tpl = DataSrv->LoadPRM("EntityTpls:" + TplName + ".prm");
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

	OK;
}
//---------------------------------------------------------------------

/*
//!!!need to know what view to test!
void CGameServer::UpdateMouseIntersectionInfo()
{
	//!!!DBG TMP!
	return;

	CStrID OldEntityUnderMouse = EntityUnderMouse;

	if (UISrv->IsMouseOverGUI() || ActiveLevel.IsNullPtr()) HasMouseIsect = false;
	else
	{
		float XRel, YRel;
		InputSrv->GetMousePosRel(XRel, YRel);
		HasMouseIsect = ActiveLevel->GetIntersectionAtScreenPos(XRel, YRel, &MousePos3D, &EntityUnderMouse);
	}

	if (!HasMouseIsect)
	{
		EntityUnderMouse = CStrID::Empty;
		MousePos3D.set(0.0f, 0.0f, 0.0f);
	}

	if (OldEntityUnderMouse != EntityUnderMouse)
	{
		Game::CEntity* pEntityUnderMouse = EntityMgr->GetEntity(OldEntityUnderMouse);
		if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseLeave"));
		pEntityUnderMouse = GetEntityUnderMouse();
		if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseEnter"));
	}
}
//---------------------------------------------------------------------

bool CGameServer::SetActiveLevel(CStrID ID)
{
	PGameLevel NewLevel;
	if (ID.IsValid())
	{
		IPTR LevelIdx = Levels.FindIndex(ID);
		if (LevelIdx == INVALID_INDEX) FAIL;
		NewLevel = Levels.ValueAt(LevelIdx);
	}

	if (NewLevel != ActiveLevel)
	{
		EventSrv->FireEvent(CStrID("OnActiveLevelChanging"));
		ActiveLevel = NewLevel;
		SetGlobalAttr<CStrID>(CStrID("ActiveLevel"), ActiveLevel.IsValidPtr() ? ID : CStrID::Empty);

		EntityUnderMouse = CStrID::Empty;
		HasMouseIsect = false;
		UpdateMouseIntersectionInfo();

		EventSrv->FireEvent(CStrID("OnActiveLevelChanged"));
	}

	OK;
}
//---------------------------------------------------------------------
*/

bool CGameLevel::OnEvent(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	return !!FireEvent(Event);
}
//---------------------------------------------------------------------

/*void CGameLevel::RenderScene()
{
//???add to each view variables?
	RenderSrv->SetAmbientLight(AmbientLight);
	RenderSrv->SetCameraPosition(MainCamera->GetPosition());
	RenderSrv->SetViewProjection(ViewProj);
*/
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

/*
//!!!???bool flags what subsystems to render?
void CGameLevel::RenderDebug()
{
	PhysicsLevel->RenderDebug();

	FireEvent(CStrID("OnRenderDebug"));

	if (SceneRoot.IsValidPtr())
	{
		Scene::CSceneNodeRenderDebug RD;
		RD.Visit(*SceneRoot);
	}
}
//---------------------------------------------------------------------
*/

//???write 2 versions, physics-based and mesh-based?
bool CGameLevel::GetIntersectionAtScreenPos(float XRel, float YRel, vector3* pOutPoint3D, CStrID* pOutEntityUID) const
{
	Frame::PNodeAttrCamera MainCamera; //!!!DBG TMP!
	if (MainCamera.IsNullPtr() || PhysicsLevel.IsNullPtr()) FAIL;

	line3 Ray;
	MainCamera->GetRay3D(XRel, YRel, 5000.f, Ray); //???ray length to far plane or infinite?

	U16 Group = PhysicsSrv->CollisionGroups.GetMask("MousePick");
	U16 Mask = PhysicsSrv->CollisionGroups.GetMask("All|MousePickTarget");
	Physics::PPhysicsObj PhysObj;
	if (!PhysicsLevel->GetClosestRayContact(Ray.Start, Ray.End(), Group, Mask, pOutPoint3D, &PhysObj)) FAIL;

	if (pOutEntityUID)
	{
		void* pUserData = PhysObj.IsValidPtr() ? PhysObj->GetUserData() : NULL;
		*pOutEntityUID = pUserData ? *(CStrID*)&pUserData : CStrID::Empty;
	}

	OK;
}
//---------------------------------------------------------------------

UPTR CGameLevel::GetEntitiesAtScreenRect(CArray<CEntity*>& Out, const Data::CRect& RelRect) const
{
	// calc frustum
	// query SPS with this frustum
	// select only render objects
	// return newly selected obj count
	Sys::Error("CGameLevel::GetEntitiesAtScreenRect() -> IMPLEMENT ME!");
	return 0;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenPos(vector2& Out, const Game::CEntity& Entity, const vector3* Offset) const
{
	Frame::PNodeAttrCamera MainCamera; //!!!DBG TMP!
	if (MainCamera.IsNullPtr()) FAIL;
	vector3 EntityPos = Entity.GetAttr<matrix44>(CStrID("Transform")).Translation();
	if (Offset) EntityPos += *Offset;
	MainCamera->GetPoint2D(EntityPos, Out.x, Out.y);
	OK;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenPosUpper(vector2& Out, const Game::CEntity& Entity) const
{
	Frame::PNodeAttrCamera MainCamera; //!!!DBG TMP!
	if (MainCamera.IsNullPtr()) FAIL;

	Prop::CPropSceneNode* pNode = Entity.GetProperty<Prop::CPropSceneNode>();
	if (!pNode) FAIL;

	CAABB AABB;
	pNode->GetAABB(AABB);
	vector3 Center = AABB.Center();
	MainCamera->GetPoint2D(vector3(Center.x, AABB.Max.y, Center.z), Out.x, Out.y);
	OK;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenRect(Data::CRect& Out, const Game::CEntity& Entity, const vector3* Offset) const
{
	Frame::PNodeAttrCamera MainCamera; //!!!DBG TMP!
	if (MainCamera.IsNullPtr()) FAIL;

	Prop::CPropSceneNode* pNode = Entity.GetProperty<Prop::CPropSceneNode>();
	if (!pNode)
	{
		matrix44 Tfm;
		if (!Entity.GetAttr(Tfm, CStrID("Transform"))) FAIL;
		float X, Y;
		MainCamera->GetPoint2D(Tfm.Translation(), X, Y);
		Out.X = (IPTR)X;
		Out.Y = (IPTR)Y;
		Out.W = 0;
		Out.H = 0;
		OK;
	}

	CAABB AABB;
	pNode->GetAABB(AABB);

	if (Offset)
	{
		AABB.Max += *Offset;
		AABB.Min += *Offset;
	}

	float X, Y;
	MainCamera->GetPoint2D(AABB.GetCorner(0), X, Y);

	float Right = X, Top = Y;
	vector2 ScreenPos;
	for (UPTR i = 1; i < 8; ++i)
	{
		MainCamera->GetPoint2D(AABB.GetCorner(i), ScreenPos.x, ScreenPos.y);

		if (ScreenPos.x < X) X = ScreenPos.x;
		else if (ScreenPos.x > Right) Right = ScreenPos.x;

		if (ScreenPos.y < Y) Y = ScreenPos.y;
		else if (ScreenPos.y > Top) Top = ScreenPos.y;
	}

	Out.X = (IPTR)X;
	Out.Y = (IPTR)Y;
	Out.W = (UPTR)(Right - X);
	Out.H = (UPTR)(Top - Y);

	OK;
}
//---------------------------------------------------------------------

UPTR CGameLevel::GetEntitiesInPhysBox(CArray<CEntity*>& Out, const matrix44& OBB) const
{
	// request physics level for shapes and bodies
	// select ones that are attached to entities
	// return newly selected obj count
	Sys::Error("CGameLevel::GetEntitiesInPhysBox() -> IMPLEMENT ME!");
	return 0;
}
//---------------------------------------------------------------------

UPTR CGameLevel::GetEntitiesInPhysSphere(CArray<CEntity*>& Out, const vector3& Center, float Radius) const
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
	U16 Group = PhysicsSrv->CollisionGroups.GetMask("Default");
	U16 Mask = PhysicsSrv->CollisionGroups.GetMask("All");
	vector3 ContactPos;
	if (!PhysicsLevel->GetClosestRayContact(Position, Position + Dir, Group, Mask, &ContactPos)) FAIL;
	Out.WorldHeight = ContactPos.y;

	//!!!material from CPhysicsObject!

	OK;
}
//---------------------------------------------------------------------

bool CGameLevel::HostsEntity(CStrID EntityID) const
{
	CEntity* pEnt = EntityMgr->GetEntity(EntityID);
	return pEnt && pEnt->GetLevel() == this;
}
//---------------------------------------------------------------------

}