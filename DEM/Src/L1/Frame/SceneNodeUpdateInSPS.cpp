#include "SceneNodeUpdateInSPS.h"

#include <Scene/SceneNode.h>
#include <Frame/RenderObject.h>
#include <Frame/Light.h>

namespace Frame
{

bool CSceneNodeUpdateInSPS::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (Attr.IsActive())
		{
			if (Attr.IsA<CRenderObject>()) ((CRenderObject&)Attr).UpdateInSPS(*pSPS);
			else if (Attr.IsA<CLight>()) ((CLight&)Attr).UpdateInSPS(*pSPS);
		}
	}

	OK;
}
//--------------------------------------------------------------------

}
