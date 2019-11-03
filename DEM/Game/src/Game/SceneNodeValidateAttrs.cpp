#include "SceneNodeValidateAttrs.h"

#include <Game/GameLevel.h>
#include <Scene/SceneNode.h>
#include <Frame/NodeAttrRenderable.h>
#include <Frame/NodeAttrSkin.h>
#include <Render/Renderable.h>

namespace Game
{

bool CSceneNodeValidateAttrs::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);
		if (Attr.IsA<Frame::CNodeAttrRenderable>())
		{
			if (!((Frame::CNodeAttrRenderable&)Attr).ValidateResources(/*need level view -> GRM instead of PGameLevel Level field*/)) FAIL;
		}
		else if (Attr.IsA<Frame::CNodeAttrSkin>())
		{
			if (!((Frame::CNodeAttrSkin&)Attr).Initialize()) FAIL;
		}
	}
	OK;
}
//--------------------------------------------------------------------

}
