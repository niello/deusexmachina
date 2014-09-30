#include "NodeAttrCollision.h"

#include <Scene/SceneNode.h>

namespace Physics
{
__ImplementClassNoFactory(Physics::CNodeAttrCollision, Scene::CNodeAttribute);

void CNodeAttrCollision::Update(const vector3* pCOIArray, DWORD COICount)
{
	CNodeAttribute::Update(pCOIArray, COICount);
	if (pNode->IsWorldMatrixChanged()) CollObj->SetTransform(pNode->GetWorldMatrix());
}
//---------------------------------------------------------------------

}
