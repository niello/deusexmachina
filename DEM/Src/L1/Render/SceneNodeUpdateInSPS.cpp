#include "SceneNodeUpdateInSPS.h"

#include <Scene/SceneNode.h>
#include <Render/RenderObject.h>
#include <Render/Light.h>

namespace Render
{

bool CSceneNodeUpdateInSPS::Visit(Scene::CSceneNode& Node)
{
	for (DWORD i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (Attr.IsActive())
		{
			if (Attr.IsA<CRenderObject>()) ((CRenderObject&)Attr).UpdateInSPS(*pSPS);
			else if (Attr.IsA<CLight>()) ((CLight&)Attr).UpdateInSPS(*pSPS);
		}
	}

	for (DWORD i = 0; i < Node.GetChildCount(); ++i)
	{
		Scene::CSceneNode* pChild = Node.GetChild(i);
		if (pChild && pChild->IsActive() && !pChild->AcceptVisitor(*this)) FAIL;
	}

	OK;
}
//--------------------------------------------------------------------

}
