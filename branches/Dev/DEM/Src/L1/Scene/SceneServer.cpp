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
void CSceneServer::Trigger()
{
	if (!pCurrScene) return;
	pCurrScene->GetRootNode().UpdateTransform();
	// Perform visibility check (attrs already added themselves to SPS)
	pCurrScene->GetRootNode().PrepareToRender();
	// Renderer must collect lights in separate array
	// Render scene (request renderer to render its tree)
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