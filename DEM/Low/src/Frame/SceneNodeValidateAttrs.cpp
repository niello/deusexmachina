#include "SceneNodeValidateAttrs.h"
#include <Scene/SceneNode.h>
#include <Frame/GraphicsResourceManager.h>
#include <Frame/NodeAttrRenderable.h>
#include <Frame/NodeAttrSkin.h>

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

		if (auto Renderable = Attr.As<Frame::CNodeAttrRenderable>())
		{
			if (!Renderable->ValidateGPUResources(_ResMgr)) FAIL;
		}
	}
	OK;
}
//--------------------------------------------------------------------

}