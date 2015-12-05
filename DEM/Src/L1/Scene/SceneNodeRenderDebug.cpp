#include "SceneNodeRenderDebug.h"

#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>

namespace Scene
{

bool CSceneNodeRenderDebug::Visit(Scene::CSceneNode& Node)
{
	if (Node.GetParent())
		DebugDraw->DrawLine(Node.GetParent()->GetWorldMatrix().Translation(), Node.GetWorldMatrix().Translation(), vector4::White);
	OK;
}
//--------------------------------------------------------------------

}
