#include "SceneNodeRenderDebug.h"

#include <Scene/SceneNode.h>
#include <Render/DebugDraw.h>

namespace Render
{

bool CSceneNodeRenderDebug::Visit(Scene::CSceneNode& Node)
{
	if (Node.GetParent())
		DebugDraw->DrawLine(Node.GetParent()->GetWorldMatrix().Translation(), Node.GetWorldMatrix().Translation(), vector4::White);

	for (DWORD i = 0; i < Node.GetChildCount(); ++i)
		if (!Node.GetChild(i)->AcceptVisitor(*this)) FAIL;

	OK;
}
//--------------------------------------------------------------------

}
