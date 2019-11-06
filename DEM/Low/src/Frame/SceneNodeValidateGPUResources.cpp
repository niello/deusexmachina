#include "SceneNodeValidateGPUResources.h"
#include <Scene/SceneNode.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/AmbientLightAttribute.h>

namespace Frame
{

bool CSceneNodeValidateGPUResources::Visit(Scene::CSceneNode& Node)
{
	//???active only?
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (auto RenderableAttr = Attr.As<CRenderableAttribute>())
		{
			if (!RenderableAttr->ValidateGPUResources(_ResMgr)) FAIL;
		}
		else if (auto LightAttr = Attr.As<CAmbientLightAttribute>())
		{
			//???must be IRenderable?
			if (!LightAttr->ValidateGPUResources(_ResMgr)) FAIL;
		}
	}

	OK;
}
//--------------------------------------------------------------------

}
