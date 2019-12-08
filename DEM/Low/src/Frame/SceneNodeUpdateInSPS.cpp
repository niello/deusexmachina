#include "SceneNodeUpdateInSPS.h"
#include <Scene/SceneNode.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/LightAttribute.h>
#include <Frame/AmbientLightAttribute.h>

namespace Frame
{

bool CSceneNodeUpdateInSPS::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (!Attr.IsActive()) continue;

		if (auto pAttr = Attr.As<CRenderableAttribute>())
			pAttr->UpdateInSPS(_SPS);
		else if (auto pAttr = Attr.As<CLightAttribute>())
			pAttr->UpdateInSPS(_SPS);
		else if (auto pAttr = Attr.As<CAmbientLightAttribute>())
			pAttr->UpdateInSPS(_SPS);
	}

	OK;
}
//--------------------------------------------------------------------

}
