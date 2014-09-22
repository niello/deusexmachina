#include "SceneNodeValidateResources.h"

#include <Scene/SceneNode.h>
#include <Render/RenderObject.h>

namespace Render
{

bool CSceneNodeValidateResources::Visit(Scene::CSceneNode& Node)
{
	for (DWORD i = 0; i < Node.GetAttrCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttr(i);
		if (Attr.IsA<CRenderObject>())
			if (!((CRenderObject*)&Attr)->ValidateResources()) FAIL;
	}

	for (DWORD i = 0; i < Node.GetChildCount(); ++i)
		if (!Node.GetChild(i)->AcceptVisitor(*this)) FAIL;

	OK;
}
//--------------------------------------------------------------------

}
