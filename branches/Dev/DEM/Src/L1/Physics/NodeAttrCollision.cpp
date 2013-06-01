#include "NodeAttrCollision.h"

#include <Physics/PhysicsWorld.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CNodeAttrCollision, Scene::CNodeAttribute);

bool CNodeAttrCollision::OnAdd()
{
	return CollObj->AttachToLevel(*pWorld);
}
//---------------------------------------------------------------------

void CNodeAttrCollision::OnRemove()
{
	CollObj->RemoveFromLevel();
}
//---------------------------------------------------------------------

void CNodeAttrCollision::Update()
{
	//???activate physics body only when tfm changed?
}
//---------------------------------------------------------------------

}
