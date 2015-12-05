#include "SceneNodeValidateResources.h"

#include <Scene/SceneNode.h>
#include <Frame/RenderObject.h>

namespace Frame
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

	OK;
}
//--------------------------------------------------------------------

}
