#include "SceneNodeValidateGPUResources.h"
#include <Scene/SceneNode.h>
#include <Frame/NodeAttrRenderable.h>
#include <Frame/NodeAttrAmbientLight.h>

namespace Frame
{

bool CSceneNodeValidateGPUResources::Visit(Scene::CSceneNode& Node)
{
	//???active only?
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (auto RenderableAttr = Attr.As<CNodeAttrRenderable>())
		{
			if (!RenderableAttr->ValidateGPUResources(_ResMgr)) FAIL;
		}
		else if (auto LightAttr = Attr.As<CNodeAttrAmbientLight>())
		{
			//???must be IRenderable?
			if (!LightAttr->ValidateGPUResources(_ResMgr)) FAIL;
		}
	}

	OK;
}
//--------------------------------------------------------------------

}
