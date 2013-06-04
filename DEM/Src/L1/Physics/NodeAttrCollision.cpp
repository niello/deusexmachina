#include "NodeAttrCollision.h"

#include <Scene/SceneNode.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CNodeAttrCollision, Scene::CNodeAttribute);

bool CNodeAttrCollision::OnAdd()
{
	return CollObj->IsAttachedToLevel() || CollObj->AttachToLevel(*pWorld);
}
//---------------------------------------------------------------------

void CNodeAttrCollision::OnRemove()
{
	CollObj->RemoveFromLevel();
}
//---------------------------------------------------------------------

void CNodeAttrCollision::Update()
{
	if (pNode->IsWorldMatrixChanged()) CollObj->SetTransform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

}
