#include "SceneNodeValidatePhysics.h"
#include <Scene/SceneNode.h>
#include <Physics/CollisionAttribute.h>
#include <Physics/PhysicsLevel.h>

namespace Physics
{

bool CSceneNodeValidatePhysics::Visit(Scene::CSceneNode& Node)
{
	for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
		if (auto pAttrTyped = Node.GetAttribute(i)->As<CCollisionAttribute>())
			pAttrTyped->SetPhysicsLevel(&_Level);
	OK;
}
//--------------------------------------------------------------------

}
