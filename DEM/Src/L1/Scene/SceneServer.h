#pragma once
#ifndef __DEM_L1_SCENE_SERVER_H__
#define __DEM_L1_SCENE_SERVER_H__

#include <Scene/Scene.h>
#include <Render/FrameShader.h>
#include <Data/Pool.h>
#include <util/ndictionary.h>

// Scene server loads and manages 3D scenes and frame shaders for a scene rendering

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

	nDictionary<CStrID, Render::CFrameShader*>	FrameShaders;

public:

	CSceneServer(): pCurrScene(NULL) { __ConstructSingleton; }
	~CSceneServer() { __DestructSingleton; }

	bool		CreateScene(CStrID Name, const bbox3& Bounds, bool SetCurrent = false);
	void		RemoveScene(CStrID Name);
	bool		SetCurrentScene(CStrID Name);
	void		SetCurrentScene(CScene* pScene);
	CStrID		GetCurrentSceneID() const { return CurrSceneID; }
	CScene*		GetCurrentScene() const { return pCurrScene; }
	CScene*		GetScene(CStrID Name);

	PSceneNode	CreateSceneNode(CStrID Name);

	void		AddFrameShader(Render::CFrameShader* pFrameShader); // nRpXmlParser xmlParser;

	void		Trigger();
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

inline PSceneNode CSceneServer::CreateSceneNode(CStrID Name)
{
	//return (CSceneNode*)NodePool.Construct();
	PSceneNode Node = n_new(CSceneNode)(Name);
	//Node->Name = Name;
	return Node;
}
//---------------------------------------------------------------------

}

#endif
