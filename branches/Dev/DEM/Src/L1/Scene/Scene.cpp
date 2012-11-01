#include "Scene.h"

#include <Scene/SceneServer.h>

namespace Scene
{

void CScene::Init(const bbox3& Bounds)
{
	RootNode = SceneSrv->CreateSceneNode(CStrID::Empty);
	RootNode->Flags.Set(CSceneNode::OwnedByScene);
}
//---------------------------------------------------------------------

void CScene::Activate()
{
}
//---------------------------------------------------------------------

void CScene::Deactivate()
{
}
//---------------------------------------------------------------------

// NB: Nodes owned by external objects and their hierarchy will persist until owner objects are destroyed
void CScene::Clear()
{
	for (int i = 0; i < OwnedNodes.Size(); ++i)
		OwnedNodes[i]->Flags.Clear(CSceneNode::OwnedByScene);
	OwnedNodes.Clear();
	RootNode->Flags.Clear(CSceneNode::OwnedByScene);
	RootNode = NULL;
}
//---------------------------------------------------------------------

}