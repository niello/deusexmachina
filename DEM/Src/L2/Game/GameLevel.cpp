#include "GameLevel.h"

#include <Game/Entity.h>
#include <Game/GameServer.h>
#include <Scripting/ScriptObject.h>
#include <Scene/SceneServer.h>		//!!!Because scene stores ScreenFrameShader! //!!!???move to RenderSrv?!
//#include <Scene/Scene.h>
//#include <Render/FrameShader.h>
#include <Scene/PropSceneNode.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsServer.h>
#include <AI/AILevel.h>
#include <Events/EventManager.h>
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
	((CGameLevel*)world->getWorldUserInfo())->FireEvent(CStrID("BeforePhysicsTick")); //???set time as param?
}
//---------------------------------------------------------------------

void PhysicsTick(btDynamicsWorld* world, btScalar timeStep)
{
	n_assert_dbg(world && world->getWorldUserInfo());
	((CGameLevel*)world->getWorldUserInfo())->FireEvent(CStrID("AfterPhysicsTick")); //???set time as param?

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
	Name = Desc.Get<nString>(CStrID("Name"), NULL);

	nString ScriptFile;
	if (Desc.Get(ScriptFile, CStrID("Script")))
	{
		Script = n_new(Scripting::CScriptObject((nString("Level_") + ID.CStr()).CStr()));
		Script->Init(); // No special class
		if (Script->LoadScriptFile(ScriptFile) == Error)
			n_printf("Error loading script for level %s\n", ID.CStr());
	}

	Data::PParams SubDesc;
	if (Desc.Get(SubDesc, CStrID("Scene")))
	{
		//!!!allow vector3 (de)serialization?!
		vector3 Center = SubDesc->Get<vector4>(CStrID("Center"), vector4::Zero);
		vector3 Extents = SubDesc->Get<vector4>(CStrID("Extents"), vector4(512.f, 128.f, 512.f, 0.f));
		int QTDepth = SubDesc->Get<int>(CStrID("QuadTreeDepth"), 3);
		bbox3 Bounds(Center, Extents);

		Scene = n_new(Scene::CScene);
		if (!Scene.IsValid()) FAIL;
		Scene->Init(Bounds, QTDepth);

		CameraManager = n_new(Scene::CCameraManager);

		Data::PParams CameraDesc;
		if (SubDesc->Get(CameraDesc, CStrID("Camera")))
		{
			bool IsThirdPerson = CameraDesc->Get(CStrID("ThirdPerson"), true);
			n_assert(IsThirdPerson); // Until a first person camera is implemented

			if (IsThirdPerson)
			{
				CameraManager->InitThirdPersonCamera(*Scene);
				Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)CameraManager->GetCameraController();
				if (pCtlr)
				{
					pCtlr->SetVerticalAngleLimits(n_deg2rad(CameraDesc->Get(CStrID("MinVAngle"), 0.0f)), n_deg2rad(CameraDesc->Get(CStrID("MaxVAngle"), 89.999f)));
					pCtlr->SetDistanceLimits(CameraDesc->Get(CStrID("MinDistance"), 0.0f), CameraDesc->Get(CStrID("MaxDistance"), 10000.0f));
					pCtlr->SetCOI(CameraDesc->Get<vector4>(CStrID("COI"), vector3::Zero)); //!!!read vector3!
					pCtlr->SetAngles(n_deg2rad(CameraDesc->Get(CStrID("VAngle"), 0.0f)), n_deg2rad(CameraDesc->Get(CStrID("HAngle"), 0.0f)));
					pCtlr->SetDistance(CameraDesc->Get(CStrID("Distance"), 20.0f));
				}
			}
		}
	}

	if (Desc.Get(SubDesc, CStrID("Physics")))
	{
		//!!!allow vector3 (de)serialization?!
		vector3 Center = SubDesc->Get<vector4>(CStrID("Center"), vector4::Zero);
		vector3 Extents = SubDesc->Get<vector4>(CStrID("Extents"), vector4(512.f, 128.f, 512.f, 0.f));
		bbox3 Bounds(Center, Extents);

		PhysWorld = n_new(Physics::CPhysicsWorld);
		if (!PhysWorld->Init(Bounds)) FAIL;

		PhysWorld->GetBtWorld()->setInternalTickCallback(PhysicsPreTick, this, true);
		PhysWorld->GetBtWorld()->setInternalTickCallback(PhysicsTick, this, false);
	}

	if (Desc.Get(SubDesc, CStrID("AI")))
	{
		//!!!allow vector3 (de)serialization?!
		vector3 Center = SubDesc->Get<vector4>(CStrID("Center"), vector4::Zero);
		vector3 Extents = SubDesc->Get<vector4>(CStrID("Extents"), vector4(512.f, 128.f, 512.f, 0.f));
		int QTDepth = SubDesc->Get<int>(CStrID("QuadTreeDepth"), 3);
		bbox3 Bounds(Center, Extents);

		AILevel = n_new(AI::CAILevel);
		if (!AILevel->Init(Bounds, QTDepth)) FAIL;

		nString NMFile;
		if (SubDesc->Get(NMFile, CStrID("NavMesh")))
			if (!AILevel->LoadNavMesh(NMFile))
				n_printf("Error loading navigation mesh for level %s\n", ID.CStr());
	}

	GlobalSub = EventMgr->Subscribe(NULL, this, &CGameLevel::OnEvent);

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::Term()
{
	GlobalSub = NULL;
	CameraManager = NULL;
	AILevel = NULL;
	PhysWorld = NULL;
	Scene = NULL;
	Script = NULL;
}
//---------------------------------------------------------------------

bool CGameLevel::Save(Data::CParams& OutDesc, const Data::CParams* pInitialDesc)
{
	// This is a chance for all properties to write their attrs to entities
	FireEvent(CStrID("OnLevelSaving"), &OutDesc);

	Data::PDataArray SGSelection = n_new(Data::CDataArray);
	for (int i = 0; i < SelectedEntities.GetCount(); ++i)
		SGSelection->Append(SelectedEntities[i]);
	OutDesc.Set(CStrID("SelectedEntities"), SGSelection);

	// Save camera state
	if (CameraManager.IsValid())
	{
		Data::PParams SGScene = n_new(Data::CParams);
		OutDesc.Set(CStrID("Scene"), SGScene);

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
				CurrCameraDesc->Set(CStrID("HAngle"), n_rad2deg(pCtlr->GetAngles().phi));
				CurrCameraDesc->Set(CStrID("VAngle"), n_rad2deg(pCtlr->GetAngles().theta));
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
	}

	// Save selection
	// Save nav. regions status

	// Save entities diff
	Data::PParams SGEntities = n_new(Data::CParams);

	Data::PParams InitialEntities;
	if (pInitialDesc && pInitialDesc->Get(InitialEntities, CStrID("Entities")))
	{
		for (int i = 0; i < InitialEntities->GetCount(); ++i)
		{
			CStrID EntityID = InitialEntities->Get(i).GetName();
			CEntity* pEntity = EntityMgr->GetEntity(EntityID, false);
			if (!pEntity || &pEntity->GetLevel() != this)
			{
				// Static objects never change, so we need no diff of them
				CStaticObject* pStaticObj = StaticEnvMgr->GetStaticObject(EntityID);
				if (!pStaticObj || &pStaticObj->GetLevel() != this)
					SGEntities->Set(EntityID, Data::CData());
			}
		}
	}

	//???is there any better way to iterate over all entities of this level? mb send them an event?
	nArray<CEntity*> Entities(128, 128);
	EntityMgr->GetEntitiesByLevel(ID, Entities);
	Data::PParams SGEntity = n_new(Data::CParams);
	const Data::CParams* pInitialEntities = InitialEntities.IsValid() && InitialEntities->GetCount() ? InitialEntities.GetUnsafe() : NULL;
	for (int i = 0; i < Entities.GetCount(); ++i)
	{
		CEntity* pEntity = Entities[i];
		if (SGEntity->GetCount()) SGEntity = n_new(Data::CParams);
		pEntity->Save(*SGEntity, pInitialEntities ? pInitialEntities->Get<Data::PParams>(pEntity->GetUID(), NULL).GetUnsafe() : NULL);
		if (SGEntity->GetCount()) SGEntities->Set(pEntity->GetUID(), SGEntity);
	}

	OutDesc.Set(CStrID("Entities"), SGEntities);

	OK;
}
//---------------------------------------------------------------------

void CGameLevel::Trigger()
{
	//!!!all events here will be fired many times if many levels exist! level as event dispatcher?

	//!!!update AI level if needed!

	if (Scene.IsValid())
	{
		Scene->ClearVisibleLists();
		Scene->GetRootNode().UpdateLocalSpace();
	}

	if (PhysWorld.IsValid())
	{
		FireEvent(CStrID("BeforePhysics"));
		PhysWorld->Trigger((float)GameSrv->GetFrameTime());
		FireEvent(CStrID("AfterPhysics"));
	}

	if (Scene.IsValid()) Scene->GetRootNode().UpdateWorldSpace();

	FireEvent(CStrID("OnWorldTfmsUpdated"));
}
//---------------------------------------------------------------------

bool CGameLevel::OnEvent(const Events::CEventBase& Event)
{
	if (((Events::CEvent&)Event).ID == CStrID("OnBeginFrame")) ProcessPendingEvents();
	return !!DispatchEvent(Event);
}
//---------------------------------------------------------------------

void CGameLevel::RenderScene()
{
	if (!Scene.IsValid()) return;

	Render::PFrameShader ScreenFrameShader = SceneSrv->GetScreenFrameShader();
	if (ScreenFrameShader.IsValid())
		Scene->Render(NULL, *ScreenFrameShader);
}
//---------------------------------------------------------------------

//!!!???separate? or with bool flags?
void CGameLevel::RenderDebug()
{
	PhysWorld->RenderDebug();

	FireEvent(CStrID("OnRenderDebug"));

	if (Scene.IsValid())
		Scene->GetRootNode().RenderDebug();
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
	if (!PhysWorld->GetClosestRayContact(Ray.start(), Ray.start() + Ray.vec(), Group, Mask, pOutPoint3D, &PhysObj)) FAIL;

	if (pOutEntityUID)
	{
		void* pUserData = PhysObj.IsValid() ? PhysObj->GetUserData() : NULL;
		*pOutEntityUID = pUserData ? *(CStrID*)&pUserData : CStrID::Empty;
	}

	OK;
}
//---------------------------------------------------------------------

DWORD CGameLevel::GetEntitiesAtScreenRect(nArray<CEntity*>& Out, const rectangle& RelRect) const
{
	// calc frustum
	// query scene quadtree with this frustum
	// select only render objects
	// return newly selected obj count
	n_error("CGameLevel::GetEntitiesAtScreenRect() -> IMPLEMENT ME!");
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

	bbox3 AABB;
	pNode->GetAABB(AABB);
	Scene->GetMainCamera().GetPoint2D(vector3(AABB.center().x, AABB.vmax.y, AABB.center().z), Out.x, Out.y);
	OK;
}
//---------------------------------------------------------------------

bool CGameLevel::GetEntityScreenRect(rectangle& Out, const Game::CEntity& Entity, const vector3* Offset) const
{
	if (!Scene.IsValid()) FAIL;

	Prop::CPropSceneNode* pNode = Entity.GetProperty<Prop::CPropSceneNode>();
	if (!pNode) FAIL;

	bbox3 AABB;
	pNode->GetAABB(AABB);

	if (Offset)
	{
		AABB.vmax += *Offset;
		AABB.vmin += *Offset;
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

DWORD CGameLevel::GetEntitiesInPhysBox(nArray<CEntity*>& Out, const matrix44& OBB) const
{
	// request physics level for shapes and bodies
	// select ones that are attached to entities
	// return newly selected obj count
	n_error("CGameLevel::GetEntitiesInPhysBox() -> IMPLEMENT ME!");
	return 0;
}
//---------------------------------------------------------------------

DWORD CGameLevel::GetEntitiesInPhysSphere(nArray<CEntity*>& Out, const vector3& Center, float Radius) const
{
	// request physics level for shapes and bodies
	// select ones that are attached to entities
	// return newly selected obj count
	n_error("CGameLevel::GetEntitiesInPhysBox() -> IMPLEMENT ME!");
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

}