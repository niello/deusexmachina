#include "SceneServer.h"

#include <gfx2/ngfxserver2.h>

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

//???what with non-current scenes? should transform be updated, or only accumulate time diff & maybe process some collision?
void CSceneServer::TriggerBeforePhysics()
{
	if (!pCurrScene) return;
	pCurrScene->GetRootNode().UpdateLocalSpace();
}
//---------------------------------------------------------------------

//???what with non-current scenes? should transform be updated, or only accumulate time diff & maybe process some collision?
void CSceneServer::TriggerAfterPhysics()
{
	if (!pCurrScene) return;

	pCurrScene->GetRootNode().UpdateWorldSpace();

	int Idx = FrameShaders.FindIndex(ScreenFrameShaderID);
	if (Idx != INVALID_INDEX)
	{
		Render::PFrameShader ScreenFrameShader = FrameShaders.ValueAtIndex(Idx);
		if (ScreenFrameShader.isvalid())
		{
			if (RenderSrv->BeginFrame()) //???or for each RT change?
			{
				pCurrScene->Render(NULL, *ScreenFrameShader);
				RenderSrv->EndFrame();
			}
		}
	}
}
//---------------------------------------------------------------------

void CSceneServer::RenderDebug()
{
	if (!pCurrScene) return;
	nGfxServer2::Instance()->BeginShapes();
	pCurrScene->GetRootNode().RenderDebug();
	nGfxServer2::Instance()->EndShapes();
}
//---------------------------------------------------------------------

}