#include "SceneNodeValidatePhysics.h"
#include <Scene/SceneNode.h>
#include <Physics/CollisionAttribute.h>

namespace Physics
{

CSceneNodeValidatePhysics::CSceneNodeValidatePhysics(Resources::CResourceManager& ResMgr, CPhysicsLevel& Level)
	: _ResMgr(ResMgr)
	, _Level(Level)
{
}
//--------------------------------------------------------------------

bool CSceneNodeValidatePhysics::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
	{
		Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);

		if (auto pAttrTyped = Attr.As<CCollisionAttribute>())
		{
			if (!pAttrTyped->ValidateResources(_ResMgr, _Level)) FAIL;
		}
	}
	OK;
}
//--------------------------------------------------------------------

}
