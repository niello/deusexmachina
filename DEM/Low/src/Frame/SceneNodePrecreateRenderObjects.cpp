#include "SceneNodePrecreateRenderObjects.h"
#include <Scene/SceneNode.h>
#include <Frame/View.h>
#include <Frame/AmbientLightAttribute.h>
#include <Frame/RenderableAttribute.h>

namespace Frame
{

bool CSceneNodePrecreateRenderObjects::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);

		if (auto pAttrTyped = Attr.As<Frame::CRenderableAttribute>())
		{
			if (!_View.GetRenderObject(*pAttrTyped)) FAIL;
		}
		else if (auto pAttrTyped = Attr.As<Frame::CAmbientLightAttribute>())
		{
			//???as renderable? or to separate cache? Use as a light type? Global IBL is much like
			//directional light, and local is much like omni with ith influence volume!
			if (!pAttrTyped->ValidateGPUResources(*_View.GetGraphicsManager())) FAIL;
		}
	}
	OK;
}
//--------------------------------------------------------------------

}
