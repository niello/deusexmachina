#include "SceneNodeRenderDebug.h"
#include <Scene/SceneNode.h>
#include <Scene/NodeAttribute.h>
#include <Debug/DebugDraw.h>

namespace Scene
{

bool CSceneNodeRenderDebug::Visit(Scene::CSceneNode& Node)
{
	if (Node.GetParent())
		_DebugDraw.DrawLine(Node.GetParent()->GetWorldMatrix().Translation(), Node.GetWorldMatrix().Translation(), vector4::White);

	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
		Node.GetAttribute(i)->RenderDebug(_DebugDraw);

	OK;
}
//--------------------------------------------------------------------

}
