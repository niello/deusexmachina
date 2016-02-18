#include "SceneNodeUpdateInSPS.h"

#include <Scene/SceneNode.h>
#include <Frame/NodeAttrRenderable.h>
#include <Frame/NodeAttrLight.h>

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
			if (Attr.IsA<CNodeAttrRenderable>()) ((CNodeAttrRenderable&)Attr).UpdateInSPS(*pSPS);
			else if (Attr.IsA<CNodeAttrLight>()) ((CNodeAttrLight&)Attr).UpdateInSPS(*pSPS);
		}
	}

	OK;
}
//--------------------------------------------------------------------

}
