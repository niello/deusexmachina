#include "SceneNodeRenderDebug.h"

#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>

namespace Scene
{

bool CSceneNodeRenderDebug::Visit(Scene::CSceneNode& Node)
{
	if (Node.GetParent())
		DebugDraw->DrawLine(Node.GetParent()->GetWorldMatrix().Translation(), Node.GetWorldMatrix().Translation(), vector4::White);

	for (DWORD i = 0; i < Node.GetChildCount(); ++i)
	{
		CSceneNode* pChild = Node.GetChild(i);
		if (pChild && pChild->IsActive() && !pChild->AcceptVisitor(*this)) FAIL;
	}

	OK;
}
//--------------------------------------------------------------------

}
