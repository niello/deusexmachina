#include "SceneNodeValidateAttrs.h"
#include <Scene/SceneNode.h>
#include <Frame/GraphicsResourceManager.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/AmbientLightAttribute.h>

namespace Frame
{

CSceneNodeValidateAttrs::CSceneNodeValidateAttrs(Frame::CGraphicsResourceManager& ResMgr)
	: _ResMgr(ResMgr)
{
	n_assert(_ResMgr.GetResourceManager());
}
//--------------------------------------------------------------------

bool CSceneNodeValidateAttrs::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);

		Attr.ValidateResources(*_ResMgr.GetResourceManager());

		if (auto pAttrTyped = Attr.As<Frame::CRenderableAttribute>())
		{
			if (!pAttrTyped->ValidateGPUResources(_ResMgr)) FAIL;
		}
		else if (auto pAttrTyped = Attr.As<Frame::CAmbientLightAttribute>())
		{
			if (!pAttrTyped->ValidateGPUResources(_ResMgr)) FAIL;
		}
	}
	OK;
}
//--------------------------------------------------------------------

}
