#include "GameLevel.h"

#include <Scene/Scene.h>
#include <Scene/SceneServer.h> //!!!only for scene creation. mb not needed!
#include <Physics/PhysicsLevel.h>
#include <AI/AILevel.h>
#include <Events/EventManager.h> //???need, or is dispatcher itself?

namespace Game
{

bool CGameLevel::Init(CStrID LevelID, const Data::CParams& Desc)
{
	//n_assert(!Initialized);

	ID = LevelID; //Desc.Get<CStrID>(CStrID("ID"), CStrID::Empty);
	Name = Desc.Get<nString>(CStrID("Name"), NULL);

	nString ScriptFile;
	if (Desc.Get(ScriptFile, CStrID("Name")))
	{
		Script = n_new(Scripting::CScriptObject(("Level_" + Name).CStr()));
		Script->Init(); // No special class
		if (Script->LoadScriptFile(ScriptFile) == Error)
			n_printf("Error loading script for level %s\n", ID.CStr());
	}

	Data::PParams SubDesc;
	if (Desc.Get(SubDesc, CStrID("Scene")))
	{
		vector3 Center = SubDesc->Get<vector3>(CStrID("Center"), vector3::Zero);
		vector3 Extents = SubDesc->Get<vector3>(CStrID("Extents"), vector3(512.f, 128.f, 512.f));
		int QTDepth = SubDesc->Get<int>(CStrID("QuadTreeDepth"), 3);
		bbox3 Bounds(Center, Extents);

		Scene = n_new(Scene::CScene);
		if (!Scene.IsValid()) FAIL;
		Scene->Init(Bounds, QTDepth);
	}

	//???desc, params?
	PhysicsLevel = n_new(Physics::CPhysicsLevel);

	//!!!OLD!
	PhysicsSrv->SetLevel(PhysicsLevel);

	if (Desc.Get(SubDesc, CStrID("AI")))
	{
		vector3 Center = SubDesc->Get<vector3>(CStrID("Center"), vector3::Zero);
		vector3 Extents = SubDesc->Get<vector3>(CStrID("Extents"), vector3(512.f, 128.f, 512.f));
		int QTDepth = SubDesc->Get<int>(CStrID("QuadTreeDepth"), 3);
		bbox3 Bounds(Center, Extents);

		AILevel = n_new(AI::CAILevel);
		if (!AILevel->Init(Bounds, QTDepth)) FAIL;

		nString NMFile;
		if (SubDesc->Get(NMFile, CStrID("NavMesh")))
			if (!AILevel->LoadNavMesh(NMFile))
				n_printf("Error loading navigation mesh for level %s\n", ID.CStr());
	}

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

	if (PhysicsLevel.IsValid())
	{
		EventMgr->FireEvent(CStrID("BeforePhysics"));
		PhysicsLevel->Trigger();
		EventMgr->FireEvent(CStrID("AfterPhysics"));
	}

	if (Scene.IsValid()) Scene->GetRootNode().UpdateWorldSpace();
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

void CGameLevel::RenderDebug()
{
	//???!!!fire from itself?!
	EventMgr->FireEvent(CStrID("OnRenderDebug"));

	if (Scene.IsValid())
		Scene->GetRootNode().RenderDebug();
}
//---------------------------------------------------------------------

}