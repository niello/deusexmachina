#include "SceneNodeValidateResources.h"

#include <Scene/SceneNode.h>
#include <Frame/NodeAttrRenderable.h>
#include <Frame/NodeAttrAmbientLight.h>
#include <Render/Renderable.h>

namespace Frame
{

bool CSceneNodeValidateResources::Visit(Scene::CSceneNode& Node)
{
	//???active only?
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (Attr.IsA<CNodeAttrRenderable>())
		{
			if (!((CNodeAttrRenderable&)Attr).GetRenderable()->ValidateResources(GPU)) FAIL;
		}
		else if (Attr.IsA<CNodeAttrAmbientLight>())
		{
			//???must be IRenderable?
			if (!((CNodeAttrAmbientLight&)Attr).ValidateResources(GPU)) FAIL;
		}
	}

	OK;
}
//--------------------------------------------------------------------

}
