#pragma once
#ifndef __DEM_L1_SCENE_SERVER_H__
#define __DEM_L1_SCENE_SERVER_H__

#include <Scene/Scene.h>
#include <Render/FrameShader.h>
#include <Animation/MocapClip.h>
#include <Resources/ResourceManager.h>
#include <Data/Pool.h>
#include <util/ndictionary.h>

// Scene server loads and manages 3D scenes and frame shaders for a scene rendering.
// Also scene server holds animation resource manager, which stores data, used for
// animation of scene nodes (including character bones).

namespace Scene
{
#define SceneSrv Scene::CSceneServer::Instance()

class CSceneServer: public Core::CRefCounted
{
	__DeclareSingleton(CSceneServer);

private:

	//CPool<CSceneNode>							NodePool;
	nDictionary<CStrID, PScene>					Scenes;
	CStrID										CurrSceneID;
	CScene*										pCurrScene;

	nDictionary<CStrID, Render::PFrameShader>	FrameShaders;	//???to RenderServer?
	CStrID										ScreenFrameShaderID;

public:

	//!!!need mgr for both anim & mocap clips, they should have the same base class!
	Resources::CResourceManager<Anim::CMocapClip>	AnimationMgr;

	CSceneServer(): pCurrScene(NULL) { __ConstructSingleton; }
	~CSceneServer() { __DestructSingleton; }

	bool		CreateScene(CStrID Name, const bbox3& Bounds, bool SetCurrent = false);
	void		RemoveScene(CStrID Name);
	bool		SetCurrentScene(CStrID Name);
	void		SetCurrentScene(CScene* pScene);
	CStrID		GetCurrentSceneID() const { return CurrSceneID; }
	CScene*		GetCurrentScene() const { return pCurrScene; }
	CScene*		GetScene(CStrID Name);

	PSceneNode	CreateSceneNode(CScene& Scene, CStrID Name);

	//???AddFrameShader to RenderServer?
	void		AddFrameShader(CStrID ID, Render::PFrameShader FrameShader); //???or always load internally?
	void		SetScreenFrameShaderID(CStrID ID) { ScreenFrameShaderID = ID; }

	void		TriggerBeforePhysics();
	void		TriggerAfterPhysics();
	void		RenderCurrentScene();
	void		RenderDebug();
};

inline CScene* CSceneServer::GetScene(CStrID Name)
{
	int Idx = Scenes.FindIndex(Name);
	return (Idx == INVALID_INDEX) ? NULL : Scenes.ValueAtIndex(Idx);
}
//---------------------------------------------------------------------

inline bool CSceneServer::SetCurrentScene(CStrID Name)
{
	int Idx = Scenes.FindIndex(Name);
	if (Idx == INVALID_INDEX) FAIL;
	SetCurrentScene(Scenes.ValueAtIndex(Idx));
	OK;
}
//---------------------------------------------------------------------

inline void CSceneServer::SetCurrentScene(CScene* pScene)
{
	if (pCurrScene) pCurrScene->Deactivate();
	pCurrScene = pScene;
	if (pCurrScene) pCurrScene->Activate();
}
//---------------------------------------------------------------------

inline PSceneNode CSceneServer::CreateSceneNode(CScene& Scene, CStrID Name)
{
	//return (CSceneNode*)NodePool.Construct();
	return n_new(CSceneNode)(Scene, Name);
}
//---------------------------------------------------------------------

inline void CSceneServer::AddFrameShader(CStrID ID, Render::PFrameShader FrameShader)
{
	n_assert(ID.IsValid() && FrameShader.isvalid());
	FrameShaders.Add(ID, FrameShader);
}
//---------------------------------------------------------------------

//???what with non-current scenes? should transform be updated, or only accumulate time diff & maybe process some collision?
inline void CSceneServer::TriggerBeforePhysics()
{
	if (pCurrScene)
	{
		pCurrScene->ClearVisibleLists();
		pCurrScene->GetRootNode().UpdateLocalSpace();
	}
}
//---------------------------------------------------------------------

//???what with non-current scenes? should transform be updated, or only accumulate time diff & maybe process some collision?
inline void CSceneServer::TriggerAfterPhysics()
{
	if (pCurrScene) pCurrScene->GetRootNode().UpdateWorldSpace();
}
//---------------------------------------------------------------------

}

#endif
