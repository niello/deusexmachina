#include "NodeAttrCollision.h"

#include <Scene/SceneNode.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CNodeAttrCollision, Scene::CNodeAttribute);

void CNodeAttrCollision::Update()
{
	if (pNode->IsWorldMatrixChanged()) CollObj->SetTransform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

}
