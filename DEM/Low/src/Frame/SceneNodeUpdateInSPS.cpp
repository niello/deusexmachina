#include "SceneNodeUpdateInSPS.h"

#include <Scene/SceneNode.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/LightAttribute.h>
#include <Frame/AmbientLightAttribute.h>

namespace Frame
{

bool CSceneNodeUpdateInSPS::Visit(Scene::CSceneNode& Node)
{
	n_assert_dbg(pSPS);

	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (Attr.IsActive())
		{
			if (Attr.IsA<CRenderableAttribute>()) ((CRenderableAttribute&)Attr).UpdateInSPS(*pSPS);
			else if (Attr.IsA<CLightAttribute>()) ((CLightAttribute&)Attr).UpdateInSPS(*pSPS);
			else if (Attr.IsA<CAmbientLightAttribute>()) ((CAmbientLightAttribute&)Attr).UpdateInSPS(*pSPS);
		}
	}

	OK;
}
//--------------------------------------------------------------------

}
