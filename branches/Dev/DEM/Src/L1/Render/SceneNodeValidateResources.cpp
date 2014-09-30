#include "SceneNodeValidateResources.h"

#include <Scene/SceneNode.h>
#include <Render/RenderObject.h>

namespace Render
{

bool CSceneNodeValidateResources::Visit(Scene::CSceneNode& Node)
{
	//???active only?
	for (DWORD i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (Attr.IsA<CRenderObject>())
			if (!((CRenderObject&)Attr).ValidateResources()) FAIL;
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
