#include "GameLevel.h"

#include <Game/Entity.h>
#include <Scripting/ScriptObject.h>
#include <Scene/SceneServer.h>		//!!!Because scene stores ScreenFrameShader! //!!!???move to RenderSrv?!
//#include <Scene/Scene.h>
//#include <Render/FrameShader.h>
#include <Scene/PropSceneNode.h>
#include <Physics/PhysicsLevel.h>
#include <AI/AILevel.h>
#include <Events/EventManager.h>

namespace Game
{

CGameLevel::~CGameLevel()
{
	Term();
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
	}

	//???desc, params?
	PhysicsLevel = n_new(Physics::CPhysicsLevel);
	PhysicsLevel->Activate();

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
	AILevel = NULL;
	PhysicsLevel = NULL;
	Scene = NULL;
	Script = NULL;
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

	if (PhysicsLevel.IsValid())
	{
		FireEvent(CStrID("BeforePhysics"));
		PhysicsLevel->Trigger();
		FireEvent(CStrID("AfterPhysics"));
	}

	if (Scene.IsValid()) Scene->GetRootNode().UpdateWorldSpace();
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
	PhysicsLevel->RenderDebug();

	FireEvent(CStrID("OnRenderDebug"));

	if (Scene.IsValid())
		Scene->GetRootNode().RenderDebug();
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

bool CGameLevel::GetSurfaceInfoUnder(CSurfaceInfo& Out, const vector3& Position, float ProbeLength /*, //!!!FILTER!*/) const
{
	n_assert(ProbeLength > 0);
	vector3 Dir(0.0f, -ProbeLength, 0.0f);

	Physics::CFilterSet ExcludeSet;
	//!!!!!if (SelfPhysicsID != -1) ExcludeSet.AddEntityID(SelfPhysicsID);
	const Physics::CContactPoint* pContact = PhysicsSrv->GetClosestContactAlongRay(Position, Dir, &ExcludeSet);
	if (pContact)
	{
		Out.WorldHeight = pContact->Position.y;
		Out.Material = pContact->Material;
		OK;
	}
	else FAIL;
}
//---------------------------------------------------------------------

}