#include "SceneServer.h"

namespace Scene
{
__ImplementSingleton(Scene::CSceneServer);

bool CSceneServer::CreateScene(CStrID Name, const bbox3& Bounds, bool SetCurrent)
{
	if (!Name.IsValid() || Scenes.Contains(Name)) FAIL;

	PScene& New = Scenes.Add(Name);
	New.Create();
	New->Init(Bounds);

	if (SetCurrent) SetCurrentScene(New);

	OK;
}
//---------------------------------------------------------------------

void CSceneServer::RemoveScene(CStrID Name)
{
	if (!Name.IsValid()) Name = CurrSceneID;
	if (!Name.IsValid()) return;
	if (Name == CurrSceneID) SetCurrentScene(NULL);
	Scenes.Erase(Name);
}
//---------------------------------------------------------------------

void CSceneServer::RenderCurrentScene()
{
	if (!pCurrScene) return;

	int Idx = FrameShaders.FindIndex(ScreenFrameShaderID);
	if (Idx != INVALID_INDEX)
	{
		Render::PFrameShader ScreenFrameShader = FrameShaders.ValueAtIndex(Idx);
		if (ScreenFrameShader.isvalid())
			pCurrScene->Render(NULL, *ScreenFrameShader);
	}
}
//---------------------------------------------------------------------

void CSceneServer::RenderDebug()
{
	if (!pCurrScene) return;
	pCurrScene->GetRootNode().RenderDebug();
}
//---------------------------------------------------------------------

}