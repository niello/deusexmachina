#include "SceneNodeRenderDebug.h"
#include <Scene/SceneNode.h>
#include <Scene/NodeAttribute.h>
#include <Debug/DebugDraw.h>

namespace Scene
{

bool CSceneNodeRenderDebug::Visit(Scene::CSceneNode& Node)
{
	// Draw nothing for the root, coordinate frames for top-level children and hierarchies for deeper ones
	if (Node.GetParent())
	{
		if (Node.GetParent()->GetParent())
			_DebugDraw.DrawLine(Node.GetParent()->GetWorldMatrix().Translation(), Node.GetWorldMatrix().Translation(), vector4::White);
		else
			_DebugDraw.DrawCoordAxes(Node.GetWorldMatrix());
	}

	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
		Node.GetAttribute(i)->RenderDebug(_DebugDraw);

	OK;
}
//--------------------------------------------------------------------

}
